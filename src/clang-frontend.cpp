///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File: clang-frontend.cpp
//
// This file is the Clang frontend implementation. Using the Clang APIs, it traverses the parsed AST
// and generates IR, which is used for optimisation and code generation.
//
// For cleanliness (and compile time reasons), no Clang/LLVM code should exist outside this file (this already takes
// long enough to compile for crying out loud).
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "frontend.h"
#include "helix.h"
#include "system.h"

#include "target-info-armv7.h"

#include <stack>

#include <stdio.h>

// Can't do anything about the compiler warnings in clangs/LLVMs
// own headers so just ignore them (they do nothing but clog up
// our own build output).
// This of course means that only clang & LLVM headers should
// be included in here, everything else should give off warnings!
//
// ****************** IGNORE WARNINGS START ******************
#pragma warning(push, 0) 

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/Tooling.h>

#include <clang/Tooling/CommonOptionsParser.h>

#include <llvm/Support/CommandLine.h>

#pragma warning(pop)
// ****************** IGNORE WARNINGS END ******************

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static llvm::cl::OptionCategory Category("helx options");
static llvm::cl::extrahelp      CommonHelp(clang::tooling::CommonOptionsParser::HelpMessage);
static llvm::cl::extrahelp      MoreHelp("\nHelix C/C++ Compiler...\n");

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct TypeInfo
{
	size_t Width; // Bytes
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CodeGenerator : public clang::RecursiveASTVisitor<CodeGenerator>
{
public:
	CodeGenerator()
		: m_TargetInfo(std::make_unique<Helix::TargetInfo_ArmV7>())
	{
		using namespace Helix;

		const TargetInfo::IntType ty = m_TargetInfo->GetSizeType();
		m_SizeType = IntegerType::Create(m_TargetInfo->GetIntBitWidth(ty));
	}

	bool VisitFunctionDecl(clang::FunctionDecl* decl);

	std::vector<Helix::Function*> GetFunctions() { return m_Functions; }

private:
	Helix::BasicBlock* CreateBasicBlock()
	{
		Helix::BasicBlock* bb = Helix::BasicBlock::Create();
		m_InstructionIterator = bb->begin();
		return bb;
	}

	void EmitBasicBlock(Helix::BasicBlock* bb)
	{
		helix_assert(m_BasicBlockIterator.is_valid(), "Cannot emit basic block - invalid iterator");
		m_BasicBlockIterator = m_CurrentFunction->InsertAfter(m_BasicBlockIterator, bb);
	}

	void EmitInsn(Helix::Instruction* insn)
	{
		helix_assert(m_InstructionIterator.is_valid(), "Cannot emit instruction - invalid iterator");
		m_InstructionIterator = m_BasicBlockIterator->InsertAfter(m_InstructionIterator, insn);
	}

	Helix::VirtualRegisterName* FindValueForDecl(clang::ValueDecl* decl)
	{
		auto it = m_ValueMap.find(decl);
		if (it == m_ValueMap.end())
			return nullptr;
		return it->second;
	}

private:
	void DoDecl(clang::Decl* decl);
	void DoStmt(clang::Stmt* stmt);
	void DoReturnStmt(clang::ReturnStmt* returnStmt);
	void DoDeclStmt(clang::DeclStmt* stmt);
	void DoVarDecl(clang::VarDecl* varDecl);
	void DoCompoundStmt(clang::CompoundStmt* compoundStmt);
	void DoIfStmt(clang::IfStmt* ifStmt);
	void DoWhileStmt(clang::WhileStmt* whileStmt);
	void DoForStmt(clang::ForStmt* forStmt);
	void DoBreakStmt(clang::BreakStmt* breakStmt);
	void DoContinueStmt(clang::ContinueStmt* continueStmt);

	Helix::Value* DoLValue(clang::Expr* expr);

	Helix::Value* DoExpr(clang::Expr* expr);
	Helix::Value* DoIntegerLiteral(clang::IntegerLiteral* integerLiteral);
	Helix::Value* DoBinOp(clang::BinaryOperator* binOp);
	Helix::Value* DoImplicitCastExpr(clang::ImplicitCastExpr* implicitCastExpr);
	Helix::Value* DoParenExpr(clang::ParenExpr* parenExpr);
	Helix::Value* DoAssignment(clang::BinaryOperator* binOp);
	Helix::Value* DoCallExpr(clang::CallExpr* callExpr);
	Helix::Value* DoUnaryOperator(clang::UnaryOperator* unaryOperator);
	Helix::Value* DoCastExpr(clang::CastExpr* castExpr);
	Helix::Value* DoArraySubscriptExpr(clang::ArraySubscriptExpr* subscriptExpr);
	Helix::Value* DoCompoundAssignOp(clang::CompoundAssignOperator* assignmentOp);

	Helix::Value* DoSizeOf(clang::QualType type);

	Helix::Value* DoScalarCast(Helix::Value* expr, clang::QualType originalType, clang::QualType requiredType);

	const Helix::Type* ConvertType(const clang::Type* type);
	const Helix::Type* ConvertType(clang::QualType type);

	const Helix::Type* ConvertBuiltinType(const clang::BuiltinType* builtinType);

	Helix::FunctionDef* LookupFunction(clang::FunctionDecl* decl);

	size_t GetAllocaElementCount(clang::QualType type);

	const Helix::Type* GetSizeType() const { return m_SizeType; }
	TypeInfo           GetTypeInfo(clang::QualType type);
	TypeInfo           GetTypeInfo(const clang::Type* typePtr);

private:
	std::vector<Helix::Function*>    m_Functions;
	Helix::Function::block_iterator  m_BasicBlockIterator;
	Helix::BasicBlock::insn_iterator m_InstructionIterator;
	Helix::Function*                 m_CurrentFunction = nullptr;

	std::stack<Helix::BasicBlock*>   m_LoopBreakStack;
	std::stack<Helix::BasicBlock*>   m_LoopContinueStack;
	std::unordered_map<clang::ValueDecl*, Helix::VirtualRegisterName*> m_ValueMap;
	std::unordered_map<clang::FunctionDecl*, Helix::FunctionDef*> m_FunctionDecls;

	std::unique_ptr<Helix::TargetInfo> m_TargetInfo;

	const Helix::Type* m_SizeType;
};

Helix::Value* CodeGenerator::DoCompoundAssignOp(clang::CompoundAssignOperator* assignmentOp)
{
	using namespace Helix;

	VirtualRegisterName* lhs = value_cast<VirtualRegisterName>(this->DoLValue(assignmentOp->getLHS()));
	helix_assert(lhs && lhs->GetType()->IsPointer(), "lhs of compound assignment is not valid lvalue");

	Value* rhs = this->DoExpr(assignmentOp->getRHS());

	Opcode opc = kInsn_Undefined;

	switch (assignmentOp->getOpcode()) {
	case clang::BO_AddAssign: opc = kInsn_IAdd; break;
	case clang::BO_SubAssign: opc = kInsn_ISub; break;
	case clang::BO_DivAssign: opc = kInsn_IDiv; break;
	case clang::BO_MulAssign: opc = kInsn_IMul; break;
	default:
		helix_unreachable("unknown compound assignment op");
	}

	const Type* resultType = this->ConvertType(assignmentOp->getComputationResultType());
	const Type* lhsType = this->ConvertType(assignmentOp->getComputationLHSType());

	VirtualRegisterName* lhsValue = VirtualRegisterName::Create(lhsType);
	VirtualRegisterName* resultValue = VirtualRegisterName::Create(resultType);

	this->EmitInsn(Helix::CreateLoad(lhs, lhsValue));
	this->EmitInsn(Helix::CreateBinOp(opc, lhsValue, rhs, resultValue));
	this->EmitInsn(Helix::CreateStore(resultValue, lhs));

	return lhs;
}

Helix::Value* CodeGenerator::DoLValue(clang::Expr* expr)
{
	switch (expr->getStmtClass()) {
	case clang::Stmt::ImplicitCastExprClass:
		return this->DoImplicitCastExpr(clang::dyn_cast<clang::ImplicitCastExpr>(expr));

	case clang::Stmt::DeclRefExprClass: {
		clang::DeclRefExpr* declRefExpr = clang::dyn_cast<clang::DeclRefExpr>(expr);
		clang::ValueDecl*   varDecl     = declRefExpr->getDecl();

		return this->FindValueForDecl(varDecl);
	}
	case clang::Stmt::UnaryOperatorClass: {
		return this->DoUnaryOperator(clang::dyn_cast<clang::UnaryOperator>(expr));
	}
	case clang::Stmt::ArraySubscriptExprClass: {
		return this->DoArraySubscriptExpr(clang::dyn_cast<clang::ArraySubscriptExpr>(expr));
	}

	case clang::Stmt::CompoundAssignOperatorClass: {
		return this->DoCompoundAssignOp(clang::cast<clang::CompoundAssignOperator>(expr));
	}

	//////////////////////////////////////////////////////////////////////////
	// HACK Alert!
	//
	// So the "contract" of DoLValue is that it will return a value with pointer
	// type in a virtual register (that referes to the given value expr).
	// This breaks that contract and returns nullptr, luckily however the only
	// case where we should need the expressions below as lvalues is when
	// they are treated as stmts and this return value is ignored anyway. Hopefully
	// at least...

	case clang::Stmt::BinaryOperatorClass:
		this->DoBinOp(clang::dyn_cast<clang::BinaryOperator>(expr));
		return nullptr;

	case clang::Stmt::CallExprClass:
		this->DoCallExpr(clang::dyn_cast<clang::CallExpr>(expr));
		return nullptr;

	// ... HACK End!
	//////////////////////////////////////////////////////////////////////////

	default:
		helix_unreachable("lvalue of unknown type");
		break;
	}

	return nullptr;
}

TypeInfo CodeGenerator::GetTypeInfo(const clang::Type* typePtr)
{
	if (const clang::BuiltinType* builtinType = clang::dyn_cast<clang::BuiltinType>(typePtr)) {
		switch (builtinType->getKind()) {
		case clang::BuiltinType::Char_S:
		case clang::BuiltinType::Char_U:
		case clang::BuiltinType::SChar:
		case clang::BuiltinType::UChar:
			return { m_TargetInfo->GetCharByteWidth() };

		case clang::BuiltinType::Short:
		case clang::BuiltinType::UShort:
			return { m_TargetInfo->GetShortByteWidth() };

		case clang::BuiltinType::Int:
		case clang::BuiltinType::UInt:
			return { m_TargetInfo->GetIntByteWidth() };

		case clang::BuiltinType::Long:
		case clang::BuiltinType::ULong:
			return { m_TargetInfo->GetLongByteWidth() };

		case clang::BuiltinType::ULongLong:
		case clang::BuiltinType::LongLong:
			return { m_TargetInfo->GetLongLongByteWidth() };
		}
	}

	if (clang::isa<clang::PointerType>(typePtr)) {
		return { m_TargetInfo->GetPointerByteWidth() };
	}

	if (const clang::ConstantArrayType* constantArrayType = clang::dyn_cast<clang::ConstantArrayType>(typePtr)) {
		const llvm::APInt& ApArraySize  = constantArrayType->getSize();
		const size_t       ArraySize    = (size_t) ApArraySize.getZExtValue();
		const size_t       ElementWidth = this->GetTypeInfo(constantArrayType->getArrayElementTypeNoTypeQual()).Width;

		return { ArraySize * ElementWidth };
	}

	helix_unreachable("GetTypeInfo(): unknown type");
	return { 0 };
}

TypeInfo CodeGenerator::GetTypeInfo(clang::QualType type)
{
	type = type.getCanonicalType();
	return this->GetTypeInfo(type.getTypePtr());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Helix::FunctionDef* CodeGenerator::LookupFunction(clang::FunctionDecl* decl)
{
	auto it = m_FunctionDecls.find(decl);
	return it != m_FunctionDecls.end() ? it->second : nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const Helix::Type* CodeGenerator::ConvertBuiltinType(const clang::BuiltinType* builtinType)
{
	switch (builtinType->getKind()) {
	case clang::BuiltinType::Char_S:
	case clang::BuiltinType::Char_U:
	case clang::BuiltinType::SChar:
	case clang::BuiltinType::UChar:
		return Helix::BuiltinTypes::GetInt8();

	case clang::BuiltinType::Short:
	case clang::BuiltinType::UShort:
		return Helix::BuiltinTypes::GetInt16();

	case clang::BuiltinType::Int:
	case clang::BuiltinType::UInt:
		return Helix::BuiltinTypes::GetInt32();

	case clang::BuiltinType::Long:
	case clang::BuiltinType::ULong:
	case clang::BuiltinType::ULongLong:
	case clang::BuiltinType::LongLong:
		return Helix::BuiltinTypes::GetInt64();

	case clang::BuiltinType::Void:
		return Helix::BuiltinTypes::GetVoidType();

	default:
		helix_unreachable("Unknown builtin type");
	}

	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const Helix::Type* CodeGenerator::ConvertType(clang::QualType type)
{
	type = type.getCanonicalType();

	return this->ConvertType(type.getTypePtr());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const Helix::Type* CodeGenerator::ConvertType(const clang::Type* type)
{
	HELIX_PROFILE_ZONE;

	if (type->isBuiltinType()) {
		return this->ConvertBuiltinType(clang::dyn_cast<clang::BuiltinType>(type));
	}

	if (type->isPointerType()) {
		return Helix::BuiltinTypes::GetPointer();
	}

	if (type->isArrayType()) {
		const clang::ArrayType* arrayType = clang::dyn_cast<clang::ArrayType>(type);
		return this->ConvertType(arrayType->getElementType());
	}

	helix_unreachable("Unknown type");

	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Helix::Value* CodeGenerator::DoSizeOf(clang::QualType type)
{
	const Helix::Type* SizeType = Helix::BuiltinTypes::GetInt64();

	const size_t TypeWidth = this->GetTypeInfo(type).Width;
	return Helix::ConstantInt::Create(SizeType, TypeWidth);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

size_t CodeGenerator::GetAllocaElementCount(clang::QualType type)
{
	type = type.getCanonicalType();

	if (!type->isArrayType()) {
		return 1;
	}

	if (const clang::ConstantArrayType* constantArray = clang::dyn_cast<clang::ConstantArrayType>(type)) {
		const llvm::APInt& apSize = constantArray->getSize();
		return (size_t) apSize.getZExtValue();
	}

	helix_unreachable("Unknown array type");
	return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Helix::Value* CodeGenerator::DoArraySubscriptExpr(clang::ArraySubscriptExpr* subscriptExpr)
{
	using namespace Helix;

	clang::Expr* baseExpr = subscriptExpr->getBase();
	clang::Expr* indexExpr = subscriptExpr->getIdx();

	Value* base = this->DoLValue(baseExpr);
	Value* index = this->DoExpr(indexExpr);

	helix_assert(base->IsA<VirtualRegisterName>(), "base is not a virtual register name");
	helix_assert(base->GetType()->IsPointer(), "base is not of pointer type");

	const clang::QualType pointerType = baseExpr->getType();
	helix_assert(pointerType->isPointerType(), "array subscript, base is not a pointer");

	const clang::QualType pointeeType = pointerType->getPointeeType();

	const Type* baseType = this->ConvertType(pointeeType);

	VirtualRegisterName* output = VirtualRegisterName::Create(BuiltinTypes::GetPointer());

	this->EmitInsn(Helix::CreateLoadEffectiveAddress(baseType, base, index, output));

	return output;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Helix::Value* CodeGenerator::DoScalarCast(Helix::Value* expr, clang::QualType originalType, clang::QualType requiredType)
{
	using namespace Helix;

	requiredType = requiredType.getCanonicalType();
	originalType = originalType.getCanonicalType();

	const Type* srcType = this->ConvertType(originalType);
	const Type* dstType = this->ConvertType(requiredType);

	if (srcType->IsIntegral()) {

		// #FIXME(bwilks): This doesn't handle any overflow/underflow cases, and it should

		// const IntegerType* srcIntegerType = type_cast<IntegerType>(srcType);

		if (dstType->IsIntegral()) {
			const IntegerType* dstIntegerType = type_cast<IntegerType>(srcType);

			if (ConstantInt* cint = value_cast<ConstantInt>(expr)) {
				if (cint->CanFitInType(dstIntegerType)) {
					expr->SetType(dstType);
				} else {
					helix_unreachable("Can't fit integer constant in required type");
				}
			} else {
				helix_unreachable("Unsupported cast expression operand");
			}

			return expr;
		}
	}

	helix_unreachable("Cannot cast between given types");

	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Helix::Value* CodeGenerator::DoCastExpr(clang::CastExpr* castExpr)
{
	clang::Expr* subExpr = castExpr->getSubExpr();

	switch (castExpr->getCastKind()) {
	case clang::CK_LValueToRValue: {
		Helix::Value* lvalue = this->DoLValue(subExpr);
		helix_assert(lvalue->IsA<Helix::VirtualRegisterName>() && lvalue->GetType()->IsPointer(), "bad lvalue");
		const Helix::Type* ty = this->ConvertType(subExpr->getType());
		Helix::VirtualRegisterName* vreg = Helix::VirtualRegisterName::Create(ty);
		this->EmitInsn(Helix::CreateLoad(Helix::value_cast<Helix::VirtualRegisterName>(lvalue), vreg));
		return vreg;
	}

	case clang::CK_ArrayToPointerDecay: // Arrays are just pointers in the IR anyway
		return this->DoLValue(subExpr);

	case clang::CK_IntegralCast:
		return this->DoScalarCast(this->DoExpr(subExpr), subExpr->getType(), castExpr->getType());

	default:
		helix_unreachable("Unknown cast kind");
		break;
	}

	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Helix::Value* CodeGenerator::DoUnaryOperator(clang::UnaryOperator* unaryOperator)
{
	using namespace Helix;

	clang::Expr* subExpr = unaryOperator->getSubExpr();

	switch (unaryOperator->getOpcode()) {
	case clang::UO_PreInc:
	case clang::UO_PostInc:
	case clang::UO_PreDec:
	case clang::UO_PostDec: {
		VirtualRegisterName* ptr = value_cast<VirtualRegisterName>(this->DoLValue(subExpr));

		helix_assert(
			ptr && ptr->GetType()->IsPointer(),
			"lvalue should evaluate to ptr vreg"
		);

		const Type* subExprType = this->ConvertType(subExpr->getType());
		
		VirtualRegisterName* v      = VirtualRegisterName::Create(subExprType);
		VirtualRegisterName* result = VirtualRegisterName::Create(subExprType);

		this->EmitInsn(Helix::CreateLoad(ptr, v));

		ConstantInt* one = ConstantInt::Create(subExprType, 1);

		if (unaryOperator->isIncrementOp()) {
			this->EmitInsn(Helix::CreateBinOp(kInsn_IAdd, v, one, result));
		} else if (unaryOperator->isDecrementOp()) {
			this->EmitInsn(Helix::CreateBinOp(kInsn_ISub, v, one, result));
		}

		this->EmitInsn(Helix::CreateStore(result, ptr));

		if (unaryOperator->isPrefix()) {
			return result;
		} else if (unaryOperator->isPostfix()) {
			return v;
		}

		break;
	}
	
	case clang::UO_Deref: {
		Value* value = this->DoExpr(subExpr);

		VirtualRegisterName* exprReg = value_cast<VirtualRegisterName>(value);

		helix_assert(exprReg, "deref expression type is not vreg");
		helix_assert(value->GetType()->IsPointer(), "cannot dereference non pointer type");
		helix_assert(subExpr->getType()->isPointerType(), "clang: sub expr not pointer");

		return exprReg;
	}

	case clang::UO_AddrOf: {
		return this->DoLValue(subExpr);
	}

	default:
		helix_unreachable("Unknown unary operator");
		break;
	}

	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Helix::Value* CodeGenerator::DoCallExpr(clang::CallExpr* callExpr)
{
	clang::FunctionDecl* functionDecl = callExpr->getDirectCallee();

	helix_assert(functionDecl, "Only direct call functions supported");

	const Helix::Type* returnType = this->ConvertType(functionDecl->getReturnType());

	Helix::Value* retValue = Helix::UndefValue::Get(returnType);
	Helix::FunctionDef* fn = this->LookupFunction(functionDecl);

	if (!functionDecl->getReturnType()->isVoidType()) {
		retValue = Helix::VirtualRegisterName::Create(returnType);
	}

	Helix::ParameterList params;

	for (clang::Expr* argExpr : callExpr->arguments()) {
		params.push_back(this->DoExpr(argExpr));
	}

	this->EmitInsn(Helix::CreateCall(fn, retValue, params));
	return retValue;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CodeGenerator::DoContinueStmt(clang::ContinueStmt* continueStmt)
{
	(void) continueStmt;

	Helix::BasicBlock* branchTarget = m_LoopContinueStack.top();
	this->EmitInsn(Helix::CreateUnconditionalBranch(branchTarget));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CodeGenerator::DoBreakStmt(clang::BreakStmt* breakStmt)
{
	(void) breakStmt;

	Helix::BasicBlock* breakTarget = m_LoopBreakStack.top();
	this->EmitInsn(Helix::CreateUnconditionalBranch(breakTarget));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CodeGenerator::DoForStmt(clang::ForStmt* forStmt)
{
	using namespace Helix;

	this->DoStmt(forStmt->getInit());

	BasicBlock* conditionBlock = BasicBlock::Create();
	BasicBlock* bodyBlock = BasicBlock::Create();
	BasicBlock* tailBlock = BasicBlock::Create();

	this->EmitInsn(Helix::CreateUnconditionalBranch(conditionBlock));

	this->EmitBasicBlock(conditionBlock);
	m_InstructionIterator = conditionBlock->begin();

	Value* conditionValue = this->DoExpr(forStmt->getCond());
	this->EmitInsn(Helix::CreateConditionalBranch(bodyBlock, tailBlock, conditionValue));

	m_LoopBreakStack.push(tailBlock);
	m_LoopContinueStack.push(conditionBlock);

	this->EmitBasicBlock(bodyBlock);
	m_InstructionIterator = bodyBlock->begin();

	this->DoStmt(forStmt->getBody());
	this->DoExpr(forStmt->getInc());

	m_LoopBreakStack.pop();
	m_LoopContinueStack.pop();

	this->EmitInsn(Helix::CreateUnconditionalBranch(conditionBlock));

	this->EmitBasicBlock(tailBlock);
	m_InstructionIterator = tailBlock->begin();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CodeGenerator::DoWhileStmt(clang::WhileStmt* whileStmt)
{
	using namespace Helix;

	BasicBlock* head = BasicBlock::Create();
	BasicBlock* body = BasicBlock::Create();
	BasicBlock* tail = BasicBlock::Create();

	this->EmitInsn(Helix::CreateUnconditionalBranch(head));

	m_InstructionIterator = head->begin();

	Value* cond = this->DoExpr(whileStmt->getCond());
	this->EmitInsn(Helix::CreateConditionalBranch(body, tail, cond));

	this->EmitBasicBlock(head);

	m_LoopBreakStack.push(tail);
	m_LoopContinueStack.push(head);

	this->EmitBasicBlock(body);
	m_InstructionIterator = body->begin();
	this->DoStmt(whileStmt->getBody());

	m_LoopBreakStack.pop();
	m_LoopContinueStack.pop();

	this->EmitInsn(Helix::CreateUnconditionalBranch(head));

	this->EmitBasicBlock(tail);
	m_InstructionIterator = tail->begin();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CodeGenerator::DoIfStmt(clang::IfStmt* ifStmt)
{
	using namespace Helix;

	Value* conditionResultRegister = this->DoExpr(ifStmt->getCond());

	BasicBlock* thenBB = BasicBlock::Create();
	BasicBlock* tailBB = BasicBlock::Create();
	BasicBlock* elseBB = tailBB;

	if (ifStmt->getElse()) {
		elseBB = BasicBlock::Create();
	}

	this->EmitInsn(Helix::CreateConditionalBranch(thenBB, elseBB, conditionResultRegister));

	// Handle 'then' part of the if (e.g. what's executed when the condition is true)
	{
 		this->EmitBasicBlock(thenBB);
		m_InstructionIterator = thenBB->begin();
		this->DoStmt(ifStmt->getThen());

		if (!Helix::IsTerminator(m_InstructionIterator->GetOpcode())) {
			this->EmitInsn(Helix::CreateUnconditionalBranch(tailBB));
		}
	}

	// Handle 'else' part of the if (e.g. what's executed when the condition is false)
	if (ifStmt->getElse()) {
		this->EmitBasicBlock(elseBB);
		m_InstructionIterator = elseBB->begin();
		this->DoStmt(ifStmt->getElse());

		if (!Helix::IsTerminator(m_InstructionIterator->GetOpcode())) {
			this->EmitInsn(Helix::CreateUnconditionalBranch(tailBB));
		}
	}
	
	this->EmitBasicBlock(tailBB);
	m_InstructionIterator = tailBB->begin();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CodeGenerator::DoDecl(clang::Decl* decl)
{
	switch (decl->getKind()) {
	case clang::Decl::Var:
		this->DoVarDecl(clang::dyn_cast<clang::VarDecl>(decl));
		break;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CodeGenerator::DoCompoundStmt(clang::CompoundStmt* compoundStmt)
{
	for (clang::Stmt* stmt : compoundStmt->body())
		this->DoStmt(stmt);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CodeGenerator::DoStmt(clang::Stmt* stmt)
{
	switch (stmt->getStmtClass()) {
	case clang::Stmt::CompoundStmtClass:
		this->DoCompoundStmt(clang::dyn_cast<clang::CompoundStmt>(stmt));
		break;

	case clang::Stmt::ReturnStmtClass:
		this->DoReturnStmt(clang::dyn_cast<clang::ReturnStmt>(stmt));
		break;

	case clang::Stmt::DeclStmtClass:
		this->DoDeclStmt(clang::dyn_cast<clang::DeclStmt>(stmt));
		break;

	case clang::Stmt::IfStmtClass:
		this->DoIfStmt(clang::dyn_cast<clang::IfStmt>(stmt));
		break;

	case clang::Stmt::WhileStmtClass:
		this->DoWhileStmt(clang::dyn_cast<clang::WhileStmt>(stmt));
		break;

	case clang::Stmt::ForStmtClass:
		this->DoForStmt(clang::dyn_cast<clang::ForStmt>(stmt));
		break;

	case clang::Stmt::BreakStmtClass:
		this->DoBreakStmt(clang::dyn_cast<clang::BreakStmt>(stmt));
		break;

	case clang::Stmt::ContinueStmtClass:
		this->DoContinueStmt(clang::dyn_cast<clang::ContinueStmt>(stmt));
		break;
	
	//////////////////////////////////////////////////////////////////////////
	// Little trick taken from clang to cover all expressions that may be
	// used in the content of a statment. We don't really care about the
	// result of these expressions, so just emit them as lvalues and ignore
	// the result
	//
	// Because of this, a little hack is needed, see CodeGenerator::DoLValue
	// for more details

#define STMT(Type, Base)
#define ABSTRACT_STMT(Op)
#define EXPR(Type, Base) \
	case clang::Stmt::Type##Class:
		#include <clang/AST/StmtNodes.inc>
	{
		this->DoLValue(clang::cast<clang::Expr>(stmt));
		break;
	}

	default:
		helix_unreachable("Unknown statment type");
		break;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CodeGenerator::DoReturnStmt(clang::ReturnStmt* returnStmt)
{
	clang::Expr* retValue = returnStmt->getRetValue();

	Helix::RetInsn* retInsn = nullptr;

	if (retValue) {
		Helix::Value* value = this->DoExpr(retValue);
		retInsn = Helix::CreateRet(value);
	} else {
		retInsn = Helix::CreateRet();
	}

	this->EmitInsn(retInsn);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CodeGenerator::DoDeclStmt(clang::DeclStmt* declStmt)
{
	for (clang::Decl* decl : declStmt->decls())
		this->DoDecl(decl);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CodeGenerator::DoVarDecl(clang::VarDecl* varDecl)
{
	using namespace Helix;

	// Create a register used to store the address of this variable on the stack...
	VirtualRegisterName* variableAddressRegister = VirtualRegisterName::Create(BuiltinTypes::GetPointer());

	const Type* type = this->ConvertType(varDecl->getType());

	const size_t elementCount = this->GetAllocaElementCount(varDecl->getType());

	// ... and then actually create the instruction to allocate space for the variable.
	//
	//              #FIXME(bwilks): This needs to pass some type information so that
	//                              it knows how much space to allocate :)
	EmitInsn(Helix::CreateStackAlloc(variableAddressRegister, type, elementCount));

	if (varDecl->hasInit()) {
		// Evaluate the RHS assignment of this var decl and store it at the
		// address allocated for this stack var.
		Helix::Value* value = this->DoExpr(varDecl->getInit());
		EmitInsn(Helix::CreateStore(value, variableAddressRegister));
	}

	// Then create a mapping for this variable declaration so that later uses of this
	// variable know which address to load from.
	m_ValueMap.insert({varDecl, variableAddressRegister});
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Helix::Value* CodeGenerator::DoAssignment(clang::BinaryOperator* binOp)
{
	using namespace Helix;

	clang::Expr* lhsExpr = binOp->getLHS();
	VirtualRegisterName* dst = Helix::value_cast<VirtualRegisterName>(this->DoLValue(lhsExpr));

	helix_assert(dst, "lvalue in assignment is not of vreg type");
	helix_assert(dst->GetType()->IsPointer(), "lhs of assignment is not a pointer");

	Helix::Value* rhs = this->DoExpr(binOp->getRHS());
	EmitInsn(Helix::CreateStore(rhs, dst));

	return rhs;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Helix::Value* CodeGenerator::DoParenExpr(clang::ParenExpr* parenExpr)
{
	return this->DoExpr(parenExpr->getSubExpr());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Helix::Value* CodeGenerator::DoIntegerLiteral(clang::IntegerLiteral* integerLiteral)
{
	const llvm::APInt integerLiteralValue = integerLiteral->getValue();

	helix_assert(integerLiteralValue.getBitWidth() <= 64, "Cannot codegen for integers > 64 bits in width");

	const Helix::Integer val = Helix::Integer(integerLiteralValue.getZExtValue());
	const Helix::Type* ty = this->ConvertType(integerLiteral->getType());

	return Helix::ConstantInt::Create(ty, val);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Helix::Value* CodeGenerator::DoBinOp(clang::BinaryOperator* binOp)
{
	if (binOp->getOpcode() == clang::BO_Assign) {
		return this->DoAssignment(binOp);
	}

	Helix::Value* lhs = this->DoExpr(binOp->getLHS());
	Helix::Value* rhs = this->DoExpr(binOp->getRHS());

	Helix::Opcode opc = Helix::kInsn_Undefined;

	switch (binOp->getOpcode()) {
	case clang::BO_Add: opc = Helix::kInsn_IAdd; break;
	case clang::BO_Sub: opc = Helix::kInsn_ISub; break;
	case clang::BO_Div: opc = Helix::kInsn_IDiv; break;
	case clang::BO_Mul: opc = Helix::kInsn_IMul; break;
	case clang::BO_LT:  opc = Helix::kInsn_ICmp_Lt; break;
	case clang::BO_GT:  opc = Helix::kInsn_ICmp_Gt; break;
	case clang::BO_LE:  opc = Helix::kInsn_ICmp_Lte; break;
	case clang::BO_GE:  opc = Helix::kInsn_ICmp_Gte; break;
	case clang::BO_EQ:  opc = Helix::kInsn_ICmp_Eq; break;
	case clang::BO_NE:  opc = Helix::kInsn_ICmp_Neq; break;

	default:
		helix_unreachable("Unsupported binary expression");
		break;
	}

	const Helix::Type* resultType = this->ConvertType(binOp->getType());

	Helix::VirtualRegisterName* result = Helix::VirtualRegisterName::Create(resultType);
	Helix::Instruction* insn = nullptr;

	if (Helix::IsCompare(opc)) {
		insn = Helix::CreateCompare(opc, lhs, rhs, result);
	} else {
		insn = Helix::CreateBinOp(opc, lhs, rhs, result);
	}

	helix_assert(insn, "Null binary operator instruction");

	EmitInsn(insn);

	return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Helix::Value* CodeGenerator::DoImplicitCastExpr(clang::ImplicitCastExpr* implicitCastExpr)
{
	return this->DoCastExpr(implicitCastExpr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Helix::Value* CodeGenerator::DoExpr(clang::Expr* expr)
{
	switch (expr->getStmtClass()) {
	case clang::Stmt::IntegerLiteralClass:
		return DoIntegerLiteral(clang::dyn_cast<clang::IntegerLiteral>(expr));

	case clang::Stmt::BinaryOperatorClass: {
		return DoBinOp(clang::dyn_cast<clang::BinaryOperator>(expr));

	case clang::Stmt::ImplicitCastExprClass:
		return DoImplicitCastExpr(clang::dyn_cast<clang::ImplicitCastExpr>(expr));

	case clang::Stmt::ParenExprClass:
		return DoParenExpr(clang::dyn_cast<clang::ParenExpr>(expr));

	case clang::Stmt::UnaryOperatorClass:
		return DoUnaryOperator(clang::dyn_cast<clang::UnaryOperator>(expr));

	case clang::Stmt::CallExprClass:
		return DoCallExpr(clang::dyn_cast<clang::CallExpr>(expr));

	case clang::Stmt::UnaryExprOrTypeTraitExprClass: {
		clang::UnaryExprOrTypeTraitExpr* uett = clang::dyn_cast<clang::UnaryExprOrTypeTraitExpr>(expr);
		
		switch (uett->getKind()) {
		case clang::UETT_SizeOf:
			return DoSizeOf(uett->getArgumentType());
		default:
			helix_unreachable("unknown unary operator or type trait");
			break;
		}

		break;
	}

	default:
		helix_unreachable("Cannot codegen for unsupported expression type");
		break;
	}
	}
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CodeGenerator::VisitFunctionDecl(clang::FunctionDecl* functionDecl)
{
	HELIX_PROFILE_ZONE;

	using namespace Helix;

	const Type* returnType = this->ConvertType(functionDecl->getReturnType());


	// Create the function
	m_CurrentFunction = Function::Create(functionDecl->getNameAsString(), returnType);

	// Reset the basic block insert point so that the next basic block will be created
	// at the start of the new function.
	//
	// Create a new basic block so that we can start inserting instructions straight
	// away, without having to explicitly create one later.
	m_BasicBlockIterator = m_CurrentFunction->begin();
	EmitBasicBlock(CreateBasicBlock());

	// And clear the value map, since variables don't persist accross functions.
	m_ValueMap.clear();

	FunctionDef::ParamTypeList paramTypes;

	for (clang::ParmVarDecl* param : functionDecl->parameters()) {
		const Type*          ty            = this->ConvertType(param->getType());
		VirtualRegisterName* paramRegister = VirtualRegisterName::Create(ty);

		m_CurrentFunction->AddParameter(paramRegister);

		VirtualRegisterName* addr = VirtualRegisterName::Create(BuiltinTypes::GetPointer());

		this->EmitInsn(Helix::CreateStackAlloc(addr, ty, 1));
		this->EmitInsn(Helix::CreateStore(paramRegister, addr));

		m_ValueMap.insert({param, addr});
		paramTypes.push_back(ty);
	}

	m_FunctionDecls.insert({
		functionDecl,
		FunctionDef::Create(
			functionDecl->getNameAsString(),
			returnType,
			paramTypes
		)
	});

	// Add the new function to the list of functions that we've generated code for in
	// this translation unit.
	m_Functions.push_back(m_CurrentFunction);

	if (functionDecl->hasBody())
		this->DoStmt(functionDecl->getBody());


	if (m_CurrentFunction->GetCountBlocks() > 1) {
		// If we have more than one basic block in a function
		// and the last one is empty & not used we can delete it.
		//
		// This tends to come from the fact that conditional constructs
		// always emit tail blocks, even if they don't end up getting used.

		if (m_BasicBlockIterator->CanDelete()) {
			m_CurrentFunction->Remove(m_BasicBlockIterator);

			BasicBlock* bb = &*m_BasicBlockIterator;
			BasicBlock::Destroy(bb);

			m_BasicBlockIterator.invalidate();
		}
	} else {
		helix_assert(m_CurrentFunction->GetCountBlocks() == 1, "Function does not contain any blocks");

		// Otherwise (we have one block) so check if its empty, or doesn't
		// contain a ret.
		// This might come from empty functions.
		if (m_BasicBlockIterator->IsEmpty() || !m_InstructionIterator->IsTerminator()) {
			if (functionDecl->getReturnType()->isVoidType()) {
				this->EmitInsn(Helix::CreateRet());
			} else {
				// It's not really valid to return a void value here, but if you've
				// not returned a value from a non void function then what are you doing
				// anyway?!?!
				this->EmitInsn(Helix::CreateRet(Helix::UndefValue::Get(returnType)));
			}
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CodeGenerator_ASTConsumer : public clang::ASTConsumer
{
public:
	virtual void HandleTranslationUnit(clang::ASTContext& ctx);

private:
	CodeGenerator m_CodeGen;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CodeGenerator_ASTConsumer::HandleTranslationUnit(clang::ASTContext& ctx)
{
	HELIX_PROFILE_ZONE;

	m_CodeGen.TraverseDecl(ctx.getTranslationUnitDecl());

	const std::vector<Helix::Function*> functions = m_CodeGen.GetFunctions();

	for (Helix::Function* fn : functions) {
		Helix::DebugDump(*fn);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ParserAction : public clang::ASTFrontendAction
{
public:
	virtual std::unique_ptr<clang::ASTConsumer>
		CreateASTConsumer(clang::CompilerInstance& ci, clang::StringRef inFile);
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<clang::ASTConsumer> ParserAction::CreateASTConsumer(clang::CompilerInstance& ci, clang::StringRef inFile)
{
	HELIX_PROFILE_ZONE;

	(void) ci; (void) inFile;

	return std::make_unique<CodeGenerator_ASTConsumer>();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Helix::Frontend::Initialise()
{
	HELIX_PROFILE_ZONE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Helix::Frontend::Run(int argc, const char** argv)
{
	HELIX_PROFILE_ZONE;

	auto expectedOptionsParser = clang::tooling::CommonOptionsParser::create(
		argc, argv, Category
	);

	if (!expectedOptionsParser) {
		llvm::errs() << expectedOptionsParser.takeError();
		return;
	}

	{
		HELIX_PROFILE_ZONE;
		
		clang::tooling::CommonOptionsParser& optionsParser = expectedOptionsParser.get();

		clang::tooling::ClangTool tool(
			optionsParser.getCompilations(),
			optionsParser.getSourcePathList()
		);

		tool.run(clang::tooling::newFrontendActionFactory<ParserAction>().get());
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Helix::Frontend::Shutdown()
{
	HELIX_PROFILE_ZONE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
