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

#include "clang-frontend.h"

#include "../helix.h"
#include "../system.h"
#include "../target-info-armv7.h"
#include "../helix-config.h"

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
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Basic/Version.h>

#include <clang/Tooling/CommonOptionsParser.h>

#include <llvm/Support/CommandLine.h>

#pragma warning(pop)
// ****************** IGNORE WARNINGS END ******************

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static llvm::cl::OptionCategory HelixGeneralOptionsCategory("General Options");

#define ARGUMENT(type,def,varName,cliName,descstr) \
	static llvm::cl::opt<type> s_Opt##varName(cliName, llvm::cl::desc(descstr), llvm::cl::cat(HelixGeneralOptionsCategory), llvm::cl::init(def)); \
	type Helix::Options::Get##varName() { return s_Opt##varName.getValue(); }

#define ARGUMENT_LIST(type,varName,cliName,descstr) \
	static llvm::cl::list<type> s_Opt##varName(cliName, llvm::cl::desc(descstr), llvm::cl::cat(HelixGeneralOptionsCategory)); \
	type Helix::Options::Get##varName(size_t index) { return s_Opt##varName[index]; } \
	size_t Helix::Options::GetCount##varName##s() { return s_Opt##varName.size(); } \

#define ARGUMENTS_POSITIONAL(type, varName, descstr) \
	static llvm::cl::list<type> s_Opt##varName(llvm::cl::Positional, llvm::cl::desc(descstr), llvm::cl::cat(HelixGeneralOptionsCategory), llvm::cl::Required); \
	type Helix::Options::Get##varName(size_t index) { return s_Opt##varName[index]; } \
	size_t Helix::Options::GetCount##varName##s() { return s_Opt##varName.size(); } \

	#include "../options.def"

static clang::ASTContext* g_GlobalASTContext;
static Helix::Module*     g_TranslationUnit = nullptr;

static const char* GetFileNameFromPath(const char* path)
{
	int lastSeperatorPos = -1;

	const char* head = path;

	for (char p = *head; p != '\0'; p = *(++head)) {
		if (p == '\\' || p == '/')
			lastSeperatorPos = (int) (head - path);
	}

	return path + lastSeperatorPos + 1;
}

static std::string FormatAssertAt(const std::string& reason, const clang::SourceLocation& loc)
{
	clang::SourceManager& sm = g_GlobalASTContext->getSourceManager();
	clang::PresumedLoc ploc = sm.getPresumedLoc(loc);

	if (ploc.isInvalid())
		return reason;

	const char* fileName = GetFileNameFromPath(ploc.getFilename());
	int line = ploc.getLine();
	int col = ploc.getColumn();

	return fmt::format("{} ({}:{}:{})", reason, fileName, line, col);
}

#define frontend_assert helix_assert
#define frontend_unreachable helix_unreachable
#define frontend_unimplemented helix_unimplemented
#define frontend_assert_at(cond, reason, where) helix_assert(cond, FormatAssertAt(reason, where))
#define frontend_unimplemented_at(reason, where) helix_unimplemented(FormatAssertAt(reason, where))

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct TypeInfo
{
	size_t Width; // Bytes
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CodeGenerator
{
public:
	CodeGenerator()
		: m_TargetInfo(std::make_unique<Helix::TargetInfo_ArmV7>())
	{ }

	void Initialise()
	{
		using namespace Helix;

		const TargetInfo::IntType ty = m_TargetInfo->GetSizeType();

		helix_assert(m_TargetInfo->GetIntBitWidth(ty) == 32, "unexpected size type");
		m_SizeType = BuiltinTypes::GetInt32();  // IntegerType::Create(m_TargetInfo->GetIntBitWidth(ty));

		clang::SourceManager& sm = g_GlobalASTContext->getSourceManager();

		const llvm::StringRef filenameRef = sm.getFileEntryForID(sm.getMainFileID())->getName();
		const std::string filename = filenameRef.str();

		helix_info(logs::general, "Creating module from '{}'", filename);

		m_Module = Helix::CreateModule(filename);

	}

	void CodeGenTranslationUnit(clang::TranslationUnitDecl* tuDecl)
	{
		for (clang::Decl* decl : tuDecl->decls()) {
			if (clang::VarDecl* topLevelVarDecl = clang::dyn_cast<clang::VarDecl>(decl)) {
				this->DoGlobalVariable(topLevelVarDecl);				
				continue;
			}

			this->DoDecl(decl);
		}
	}

	Helix::Module* GetModule() const { return m_Module; }

private:
	Helix::BasicBlock* CreateBasicBlock()
	{
		Helix::BasicBlock* bb = Helix::BasicBlock::Create();
		m_InstructionIterator = bb->begin();
		return bb;
	}

	void EmitBasicBlock(Helix::BasicBlock* bb)
	{
		frontend_assert(m_BasicBlockIterator.is_valid(), "Cannot emit basic block - invalid iterator");
		m_BasicBlockIterator = m_CurrentFunction->InsertAfter(m_BasicBlockIterator, bb);
	}

	void EmitInsn(Helix::Instruction* insn)
	{
		frontend_assert(m_InstructionIterator.is_valid(), "Cannot emit instruction - invalid iterator");
		m_InstructionIterator = m_BasicBlockIterator->InsertAfter(m_InstructionIterator, insn);
	}

	Helix::Value* FindValueForDecl(clang::ValueDecl* decl)
	{
		auto it = m_ValueMap.find(decl);

		if (it == m_ValueMap.end()) {
			auto git = m_GlobalVars.find(decl);
			
			if (git == m_GlobalVars.end())
				return nullptr;

			return git->second;
		}

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
	void DoDoLoop(clang::DoStmt* doStmt);
	void DoGoto(clang::GotoStmt* gotoStmt);
	void DoLabel(clang::LabelStmt* labelStmt);
	void DoRecordDecl(clang::RecordDecl* recordDecl);
	void DoFunctionDecl(clang::FunctionDecl* functionDecl);
	void DoGlobalVariable(clang::VarDecl* varDecl);

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
	Helix::Value* DoMemberExpr(clang::MemberExpr* memberExpr);
	Helix::Value* DoCharacterLiteral(clang::CharacterLiteral* characterLiteral);
	Helix::Value* DoInitListExpr(clang::InitListExpr* initListExpr);
	Helix::Value* DoLocalStringLiteralExpr(clang::StringLiteral* stringLiteral);
	Helix::Value* DoStringLiteral(clang::StringLiteral* stirngLiteral);

	Helix::Value* DoSizeOf(clang::QualType type);


	Helix::Value* DoScalarCast(Helix::Value* expr, clang::QualType originalType, clang::QualType requiredType);

	const Helix::Type* ConvertType(const clang::Type* type);
	const Helix::Type* ConvertType(clang::QualType type);

	const Helix::Type* ConvertBuiltinType(const clang::BuiltinType* builtinType);

	Helix::Function* LookupFunction(clang::FunctionDecl* decl);

	size_t GetArrayElementCount(const clang::ArrayType* type);

	const Helix::Type* GetSizeType() const { return m_SizeType; }
	TypeInfo           GetTypeInfo(clang::QualType type);
	TypeInfo           GetTypeInfo(const clang::Type* typePtr);

	Helix::Integer EvaluteConstantIntegralExpression(clang::Expr* expr);

private:
	Helix::Module*                   m_Module;
	Helix::Function::block_iterator  m_BasicBlockIterator;
	Helix::BasicBlock::insn_iterator m_InstructionIterator;
	Helix::Function*                 m_CurrentFunction = nullptr;

	std::stack<Helix::BasicBlock*>   m_LoopBreakStack;
	std::stack<Helix::BasicBlock*>   m_LoopContinueStack;
	std::unordered_map<clang::ValueDecl*, Helix::VirtualRegisterName*> m_ValueMap;
	std::unordered_map<clang::ValueDecl*, Helix::GlobalVariable*> m_GlobalVars;
	std::unordered_map<clang::FunctionDecl*, Helix::Function*> m_FunctionDecls;

	std::unique_ptr<Helix::TargetInfo> m_TargetInfo;

	std::unordered_map<clang::LabelDecl*, Helix::BasicBlock*> m_Labels;
	std::unordered_map<const clang::Type*, const Helix::StructType*> m_Records;

	const Helix::Type* m_SizeType;
};

Helix::Value* CodeGenerator::DoStringLiteral(clang::StringLiteral* stringLiteral)
{
	helix_assert(stringLiteral->getKind() == clang::StringLiteral::Ascii, "only ascii string literals are supported");
	helix_assert(stringLiteral->getCharByteWidth() == 1, "only 'char*' strings are supported");

	std::vector<uint8_t> characters;
	characters.reserve(stringLiteral->getByteLength() + 1);

	for (size_t i = 0; i < stringLiteral->getByteLength(); ++i) {
		characters.push_back((char) stringLiteral->getCodeUnit(i));
	}

	characters.push_back('\0');

	const Helix::ArrayType* at = Helix::ArrayType::Create(characters.size(), Helix::BuiltinTypes::GetInt8());
	Helix::ConstantByteArray* ca = Helix::ConstantByteArray::Create(characters, at);
	return ca;
}

Helix::Value* CodeGenerator::DoLocalStringLiteralExpr(clang::StringLiteral* stringLiteral)
{
	Helix::Value* ca = DoStringLiteral(stringLiteral);

	static size_t s_StringIndex = 0;
	const std::string name = "str." + std::to_string(s_StringIndex);
	s_StringIndex++;

	Helix::GlobalVariable* gvar = Helix::GlobalVariable::Create(name, ca->GetType(), ca);
	m_Module->RegisterGlobalVariable(gvar);
	return gvar;
}

Helix::Value* CodeGenerator::DoInitListExpr(clang::InitListExpr* initListExpr)
{
	if (initListExpr->getType()->isArrayType()) {
		std::vector<Helix::Value*> inits;
		inits.reserve(initListExpr->getNumInits());

		for (clang::Expr* init : initListExpr->inits())
			inits.push_back(this->DoExpr(init));

		const Helix::ArrayType* ty = Helix::type_cast<Helix::ArrayType>(this->ConvertType(initListExpr->getType()));
		frontend_assert_at(ty, "init list expr not an array", initListExpr->getExprLoc());

		return Helix::ConstantArray::Create(inits, ty);
	}
	else if (initListExpr->getType()->isRecordType()) {
		std::vector<Helix::Value*> inits;
		inits.reserve(initListExpr->getNumInits());

		for (clang::Expr* init : initListExpr->inits())
			inits.push_back(this->DoExpr(init));

		const Helix::StructType* ty = Helix::type_cast<Helix::StructType>(this->ConvertType(initListExpr->getType()));
		frontend_assert_at(ty, "init list expr not a struct", initListExpr->getExprLoc());
		return Helix::ConstantStruct::Create(inits, ty);
	}

	frontend_unimplemented_at("unknown init list expr", initListExpr->getExprLoc());
	return nullptr;
}

Helix::Value* CodeGenerator::DoCharacterLiteral(clang::CharacterLiteral* characterLiteral)
{
	using namespace Helix;

	frontend_assert_at(
		characterLiteral->getKind() == clang::CharacterLiteral::Ascii,
		"only ascii character literals are supported",
		characterLiteral->getBeginLoc()
	);

	const unsigned value = characterLiteral->getValue();

	return ConstantInt::Create(BuiltinTypes::GetInt32(), Helix::Integer(value));
}

void CodeGenerator::DoGlobalVariable(clang::VarDecl* varDecl)
{
	using namespace Helix;

	const Type* baseType = this->ConvertType(varDecl->getType());

	GlobalVariable* gvar = [this, varDecl, baseType]() {
		const std::string name = varDecl->getName().str();

		if (varDecl->hasInit()) {
			if (clang::StringLiteral* stringLiteral = clang::dyn_cast<clang::StringLiteral>(varDecl->getInit()->IgnoreCasts())) {
				return GlobalVariable::Create(name, baseType, DoStringLiteral(stringLiteral));
			}
			
			Value* init = this->DoExpr(varDecl->getInit());
			
			// helix_assert(init->IsConstant(), "global var initializer is not constant value");

			return GlobalVariable::Create(name, baseType, init);
		} else {
			return GlobalVariable::Create(name, baseType);
		}
	}();

	helix_assert(m_GlobalVars.find(varDecl) == m_GlobalVars.end(), "global VarDecl is not unique");

	m_GlobalVars.insert({ varDecl, gvar });
	m_Module->RegisterGlobalVariable(gvar);
}

Helix::Integer CodeGenerator::EvaluteConstantIntegralExpression(clang::Expr* expr)
{
	frontend_assert_at(expr->getType()->isIntegerType(), "cannot evaluate constant non integral expression", expr->getExprLoc());

	switch (expr->getStmtClass()) {
	case clang::Expr::ConstantExprClass:
		return this->EvaluteConstantIntegralExpression(clang::cast<clang::ConstantExpr>(expr)->getSubExpr());

	case clang::Expr::IntegerLiteralClass: {
		clang::IntegerLiteral* p = clang::cast<clang::IntegerLiteral>(expr);
		return Helix::Integer(p->getValue().getZExtValue());
	}
	case clang::Expr::BinaryOperatorClass: {
		clang::BinaryOperator* p = clang::cast<clang::BinaryOperator>(expr);

		Helix::Integer l = EvaluteConstantIntegralExpression(p->getLHS());
		Helix::Integer r = EvaluteConstantIntegralExpression(p->getRHS());

		switch (p->getOpcode()) {
		case clang::BO_Add: return l + r;
		case clang::BO_Mul: return l * r;
		case clang::BO_Div: return l / r;
		case clang::BO_Sub: return l - r;
		case clang::BO_And: return l & r;
		case clang::BO_Or:  return l | r;
		case clang::BO_Xor: return l ^ r;
		default:
			frontend_unimplemented_at("unknown constant binary operator", expr->getExprLoc());
			break;
		}

		return 0;
	}
	case clang::Expr::UnaryExprOrTypeTraitExprClass: {
		clang::UnaryExprOrTypeTraitExpr* uett = clang::cast<clang::UnaryExprOrTypeTraitExpr>(expr);

		switch (uett->getKind()) {
		case clang::UETT_SizeOf: {
			return Helix::Integer(this->GetTypeInfo(uett->getTypeOfArgument()).Width);
		}
		default:
			frontend_unimplemented_at("unknown constant unary expr or type trait", expr->getExprLoc());
		}

		break;
	}
	case clang::Expr::ImplicitCastExprClass:
		return this->EvaluteConstantIntegralExpression(clang::cast<clang::ImplicitCastExpr>(expr)->getSubExpr());

	case clang::Expr::DeclRefExprClass: {
		clang::DeclRefExpr* p = clang::cast<clang::DeclRefExpr>(expr);
		clang::ValueDecl* valueDecl = p->getDecl();

		switch (valueDecl->getKind()) {
		case clang::ValueDecl::EnumConstant: {
			clang::EnumConstantDecl* enumConstantDecl = clang::cast<clang::EnumConstantDecl>(valueDecl);

			const Helix::Integer integralValue = [this, enumConstantDecl]() {
				if (enumConstantDecl->getInitExpr()) {
					return this->EvaluteConstantIntegralExpression(enumConstantDecl->getInitExpr());
				}
				else {
					return enumConstantDecl->getInitVal().getZExtValue();
				}
			}();

			return integralValue;
		}

		default:
			frontend_unimplemented_at("unknown DeclRefExpr here", expr->getExprLoc());
			break;
		}

		break;
	}
	default:
		frontend_unimplemented_at("unknown constant expression type", expr->getExprLoc());
		break;
	}

	return 0;
}

Helix::Value* CodeGenerator::DoMemberExpr(clang::MemberExpr* memberExpr)
{
	using namespace Helix;

	clang::Expr* base = memberExpr->getBase();

	Value* base_lvalue = this->DoLValue(base);
	helix_assert(base_lvalue && base_lvalue->GetType()->IsPointer(), "base lvalue not ptr");

	clang::NamedDecl* memberDecl = memberExpr->getMemberDecl();
	clang::FieldDecl* fieldDecl = clang::dyn_cast<clang::FieldDecl>(memberDecl);

	helix_assert(fieldDecl, "member decl not a FieldDecl");

	const Helix::StructType* baseType = type_cast<StructType>(this->ConvertType(fieldDecl->getParent()->getTypeForDecl()));
	frontend_assert_at(baseType, "base type not struct", memberExpr->getExprLoc());

	VirtualRegisterName* result = VirtualRegisterName::Create(Helix::BuiltinTypes::GetPointer());
	this->EmitInsn(Helix::CreateLoadFieldAddress(baseType, base_lvalue, fieldDecl->getFieldIndex(), result));
	return result;
}

void CodeGenerator::DoRecordDecl(clang::RecordDecl* decl)
{
	if (decl->getName() == "_GUID")
		return;

	frontend_assert_at(decl->isStruct(), "Only struct records are supported", decl->getBeginLoc());

	const clang::Type* ty = decl->getTypeForDecl();

	helix_assert(m_Records.find(ty) == m_Records.end(), "Duplicate records");

	Helix::StructType::FieldList fields;

	for (clang::FieldDecl* fieldDecl : decl->fields()) {
		const Helix::Type* fieldType = this->ConvertType(fieldDecl->getType());
		fields.push_back(fieldType);
	}

	const Helix::StructType* myType = [decl, fields](){
		if (!decl->getIdentifier()) {
			return Helix::StructType::Create(fields);
		} else {
			llvm::StringRef name = decl->getName();
			return Helix::StructType::Create(name.str(), fields);
		}
	}();

	m_Records.insert({ty, myType});
	m_Module->RegisterStruct(myType);
}

void CodeGenerator::DoGoto(clang::GotoStmt* gotoStmt)
{
	clang::LabelDecl* labelDecl = gotoStmt->getLabel();

	auto it = m_Labels.find(labelDecl);

	Helix::BasicBlock* bb = nullptr;

	if (it == m_Labels.end()) {
		bb = Helix::BasicBlock::Create();
		m_Labels.insert({labelDecl, bb});
	} else {
		bb = it->second;
	}

	if (!m_InstructionIterator->IsTerminator()) {
		this->EmitInsn(Helix::CreateUnconditionalBranch(bb));
	}
}

void CodeGenerator::DoLabel(clang::LabelStmt* labelStmt)
{
	clang::LabelDecl* labelDecl = labelStmt->getDecl();

	auto it = m_Labels.find(labelDecl);

	Helix::BasicBlock* bb = nullptr;

	if (it == m_Labels.end()) {
		bb = Helix::BasicBlock::Create();
		m_Labels.insert({labelDecl, bb});
	} else {
		bb = it->second;
	}

	if (!m_InstructionIterator->IsTerminator()) {
		this->EmitInsn(Helix::CreateUnconditionalBranch(bb));
	}

	this->EmitBasicBlock(bb);
	m_InstructionIterator = bb->begin();

	this->DoStmt(labelStmt->getSubStmt());
}

void CodeGenerator::DoDoLoop(clang::DoStmt* doStmt)
{
	using namespace Helix;

	BasicBlock* body = BasicBlock::Create();
	BasicBlock* tail = BasicBlock::Create();

	this->EmitInsn(Helix::CreateUnconditionalBranch(body));
	m_InstructionIterator = body->begin();

	{
		m_LoopBreakStack.push(tail);
		m_LoopContinueStack.push(body);

		this->EmitBasicBlock(body);
		m_InstructionIterator = body->begin();
		this->DoStmt(doStmt->getBody());

		m_LoopBreakStack.pop();
		m_LoopContinueStack.pop();
	}

	Value* cond = this->DoExpr(doStmt->getCond());
	this->EmitInsn(Helix::CreateConditionalBranch(body, tail, cond));

	this->EmitBasicBlock(tail);
	m_InstructionIterator = tail->begin();
}

Helix::Value* CodeGenerator::DoCompoundAssignOp(clang::CompoundAssignOperator* assignmentOp)
{
	using namespace Helix;

	Value* lhs = this->DoLValue(assignmentOp->getLHS());
	frontend_assert(lhs && lhs->GetType()->IsPointer(), "lhs of compound assignment is not valid lvalue");

	Value* rhs = this->DoExpr(assignmentOp->getRHS());

	HLIR::Opcode opc = HLIR::Undefined;

	switch (assignmentOp->getOpcode()) {
	case clang::BO_AddAssign: opc = HLIR::IAdd; break;
	case clang::BO_SubAssign: opc = HLIR::ISub; break;
	case clang::BO_MulAssign: opc = HLIR::IMul; break;
	case clang::BO_AndAssign: opc = HLIR::And;  break;
	case clang::BO_OrAssign:  opc = HLIR::Or;   break;
	case clang::BO_XorAssign: opc = HLIR::Xor;  break;
	case clang::BO_RemAssign: {
		if (assignmentOp->getType()->hasUnsignedIntegerRepresentation()) {
			opc = Helix::HLIR::IURem;
		} else {
			opc = Helix::HLIR::ISRem;
		}
		break;
	}

	case clang::BO_DivAssign: {
		if (assignmentOp->getType()->hasUnsignedIntegerRepresentation()) {
			opc = Helix::HLIR::IUDiv;
		} else {
			opc = Helix::HLIR::ISDiv;
		}
		break;
	}
	default:
		frontend_unimplemented_at("unknown compound assignment op", assignmentOp->getOperatorLoc());
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
	case clang::Stmt::StringLiteralClass: {
		return this->DoLocalStringLiteralExpr(clang::cast<clang::StringLiteral>(expr));
	}
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

	case clang::Stmt::MemberExprClass: {
		return this->DoMemberExpr(clang::cast<clang::MemberExpr>(expr));
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
		frontend_unimplemented_at("lvalue of unknown type", expr->getExprLoc());
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

	frontend_unimplemented(fmt::format("GetTypeInfo(): unknown type '{}'",
		clang::QualType(typePtr,0).getAsString(clang::PrintingPolicy { {} })));

	return { 0 };
}

TypeInfo CodeGenerator::GetTypeInfo(clang::QualType type)
{
	type = type.getCanonicalType();
	return this->GetTypeInfo(type.getTypePtr());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Helix::Function* CodeGenerator::LookupFunction(clang::FunctionDecl* decl)
{
	auto it = m_FunctionDecls.find(decl);
	return it != m_FunctionDecls.end() ? it->second : nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const Helix::Type* CodeGenerator::ConvertBuiltinType(const clang::BuiltinType* builtinType)
{
	// #FIXME: Convert this to use the target info system

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
	case clang::BuiltinType::Long:
	case clang::BuiltinType::ULong:
		return Helix::BuiltinTypes::GetInt32();

	case clang::BuiltinType::ULongLong:
	case clang::BuiltinType::LongLong:
		return Helix::BuiltinTypes::GetInt64();

	case clang::BuiltinType::Void:
		return Helix::BuiltinTypes::GetVoidType();

	default:
		frontend_unimplemented(fmt::format("Unknown builtin type '{0}'", builtinType->getNameAsCString(clang::PrintingPolicy { {} })));
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

		const size_t elementCount = this->GetArrayElementCount(arrayType);
		const Helix::Type* elementType = this->ConvertType(arrayType->getElementType());

		return Helix::ArrayType::Create(elementCount, elementType);
	}

	if (type->isRecordType()) {
		auto it = m_Records.find(type);
		helix_assert(it != m_Records.end(), "No such record");

		return it->second;
	}

	if (type->isEnumeralType()) {
		return Helix::BuiltinTypes::GetInt32();
	}

	frontend_unimplemented(fmt::format("Unknown type '{}'", clang::QualType(type,0).getAsString(clang::PrintingPolicy { {} })));

	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Helix::Value* CodeGenerator::DoSizeOf(clang::QualType type)
{
	const Helix::Type* sizeType = this->GetSizeType();

	const size_t TypeWidth = this->GetTypeInfo(type).Width;
	return Helix::ConstantInt::Create(sizeType, TypeWidth);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

size_t CodeGenerator::GetArrayElementCount(const clang::ArrayType* type)
{
	if (const clang::ConstantArrayType* constantArray = clang::dyn_cast<clang::ConstantArrayType>(type)) {
		const llvm::APInt& apSize = constantArray->getSize();
		return (size_t) apSize.getZExtValue();
	}

	frontend_unimplemented(fmt::format("Unknown array type '{}'", clang::QualType(type,0).getAsString(clang::PrintingPolicy { {} })));
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

	frontend_assert(base->GetType()->IsPointer(), "base is not of pointer type");

	const clang::QualType pointerType = baseExpr->getType();
	frontend_assert(pointerType->isPointerType(), "array subscript, base is not a pointer");

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

	if (srcType == dstType) {
		return expr;
	}

	if (srcType->IsIntegral()) {
		const IntegerType* srcIntegerType = type_cast<IntegerType>(srcType);
		// #FIXME(bwilks): This doesn't handle any overflow/underflow cases, and it should

		if (dstType->IsIntegral()) {
			const IntegerType* dstIntegerType = type_cast<IntegerType>(dstType);
			frontend_assert(
				srcIntegerType->GetBitWidth() != dstIntegerType->GetBitWidth(),
				"trying to cast between two integral types of the same bit width"
			);

			if (ConstantInt* cint = value_cast<ConstantInt>(expr)) {
				if (cint->CanFitInType(dstIntegerType)) {
					return ConstantInt::Create(dstIntegerType, cint->GetIntegralValue());
				} else {
					frontend_unreachable("Can't fit integer constant in required type");
				}
			} else {
				if (dstIntegerType->GetBitWidth() > srcIntegerType->GetBitWidth()) {
					VirtualRegisterName* output = VirtualRegisterName::Create(dstType);
				
					if (originalType->isUnsignedIntegerType()) {
						EmitInsn(Helix::CreateZExt(expr, output));
					}
					else {
						EmitInsn(Helix::CreateSExt(expr, output));
					}

					return output;
				} else {
					frontend_unimplemented("casts from big types to smaller types are not supported");
				}
			}

			return expr;
		}
	}

	frontend_unreachable("Cannot cast between given types");

	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Helix::Value* CodeGenerator::DoCastExpr(clang::CastExpr* castExpr)
{
	clang::Expr* subExpr = castExpr->getSubExpr();

	switch (castExpr->getCastKind()) {
	case clang::CK_LValueToRValue: {
		Helix::Value* lvalue = this->DoLValue(subExpr);
		frontend_assert(lvalue->GetType()->IsPointer(), "bad lvalue");
		const Helix::Type* ty = this->ConvertType(subExpr->getType());
		Helix::VirtualRegisterName* vreg = Helix::VirtualRegisterName::Create(ty);
		this->EmitInsn(Helix::CreateLoad(lvalue, vreg));
		return vreg;
	}

	case clang::CK_ArrayToPointerDecay: // Arrays are just pointers in the IR anyway
		return this->DoLValue(subExpr);

	case clang::CK_IntegralCast:
		return this->DoScalarCast(this->DoExpr(subExpr), subExpr->getType(), castExpr->getType());

	default:
		frontend_unimplemented_at("Unknown cast kind", castExpr->getExprLoc());
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
	case clang::UO_Not: {
		Value* v = this->DoExpr(subExpr);
		VirtualRegisterName* result = VirtualRegisterName::Create(v->GetType());
		Value* max = ConstantInt::GetMax(v->GetType());
		this->EmitInsn(Helix::CreateBinOp(HLIR::Xor, v, max, result));
		return result;
	}

	case clang::UO_PreInc:
	case clang::UO_PostInc:
	case clang::UO_PreDec:
	case clang::UO_PostDec: {
		Value* ptr = this->DoLValue(subExpr);

		frontend_assert(
			ptr && ptr->GetType()->IsPointer(),
			"lvalue should evaluate to ptr"
		);

		const Type* subExprType = this->ConvertType(subExpr->getType());
		
		VirtualRegisterName* v      = VirtualRegisterName::Create(subExprType);
		VirtualRegisterName* result = VirtualRegisterName::Create(subExprType);

		this->EmitInsn(Helix::CreateLoad(ptr, v));

		ConstantInt* one = ConstantInt::Create(subExprType, 1);

		if (unaryOperator->isIncrementOp()) {
			this->EmitInsn(Helix::CreateBinOp(HLIR::IAdd, v, one, result));
		} else if (unaryOperator->isDecrementOp()) {
			this->EmitInsn(Helix::CreateBinOp(HLIR::ISub, v, one, result));
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

		frontend_assert(value->GetType()->IsPointer(), "cannot dereference non pointer type");
		frontend_assert(subExpr->getType()->isPointerType(), "clang: sub expr not pointer");

		return value;
	}

	case clang::UO_AddrOf: {
		return this->DoLValue(subExpr);
	}

	default:
		frontend_unimplemented_at("Unknown unary operator", unaryOperator->getExprLoc());
		break;
	}

	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Helix::Value* CodeGenerator::DoCallExpr(clang::CallExpr* callExpr)
{
	clang::FunctionDecl* functionDecl = callExpr->getDirectCallee();

	frontend_assert(functionDecl, "Only direct call functions supported");

	const Helix::Type* returnType = this->ConvertType(functionDecl->getReturnType());

	Helix::Value* retValue = Helix::UndefValue::Get(returnType);
	Helix::Function* fn = this->LookupFunction(functionDecl);

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

	if (forStmt->getInit())
		this->DoStmt(forStmt->getInit());

	BasicBlock* conditionBlock = BasicBlock::Create();
	BasicBlock* bodyBlock = BasicBlock::Create();
	BasicBlock* tailBlock = BasicBlock::Create();

	this->EmitInsn(Helix::CreateUnconditionalBranch(conditionBlock));

	this->EmitBasicBlock(conditionBlock);
	m_InstructionIterator = conditionBlock->begin();

	if (forStmt->getCond()) {
		Value* conditionValue = this->DoExpr(forStmt->getCond());
		this->EmitInsn(Helix::CreateConditionalBranch(bodyBlock, tailBlock, conditionValue));
	} else {
		this->EmitInsn(Helix::CreateUnconditionalBranch(bodyBlock));
	}

	m_LoopBreakStack.push(tailBlock);
	m_LoopContinueStack.push(conditionBlock);

	this->EmitBasicBlock(bodyBlock);
	m_InstructionIterator = bodyBlock->begin();

	if (forStmt->getBody())
		this->DoStmt(forStmt->getBody());
	
	if (forStmt->getInc())
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

		if (!m_InstructionIterator->IsTerminator()) {
			this->EmitInsn(Helix::CreateUnconditionalBranch(tailBB));
		}
	}

	// Handle 'else' part of the if (e.g. what's executed when the condition is false)
	if (ifStmt->getElse()) {
		this->EmitBasicBlock(elseBB);
		m_InstructionIterator = elseBB->begin();
		this->DoStmt(ifStmt->getElse());

		if (!m_InstructionIterator->IsTerminator()) {
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
	case clang::Decl::Function:
		this->DoFunctionDecl(clang::cast<clang::FunctionDecl>(decl));
		break;

	case clang::Decl::Record:
		this->DoRecordDecl(clang::cast<clang::RecordDecl>(decl));
		break;

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
	case clang::Stmt::GotoStmtClass:
		this->DoGoto(clang::cast<clang::GotoStmt>(stmt));
		break;

	case clang::Stmt::LabelStmtClass:
		this->DoLabel(clang::cast<clang::LabelStmt>(stmt));
		break;

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

	case clang::Stmt::DoStmtClass:
		this->DoDoLoop(clang::dyn_cast<clang::DoStmt>(stmt));
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

	case clang::Stmt::NullStmtClass:
		break;

	default:
		frontend_unimplemented_at("Unknown statment type", stmt->getBeginLoc());
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

	// ... and then actually create the instruction to allocate space for the variable.
	//
	//              #FIXME(bwilks): This needs to pass some type information so that
	//                              it knows how much space to allocate :)
	EmitInsn(Helix::CreateStackAlloc(variableAddressRegister, type));

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
	Value* dst = this->DoLValue(lhsExpr);

	frontend_assert(dst && dst->GetType()->IsPointer(), "lhs of assignment is not a pointer");

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

	frontend_assert(integerLiteralValue.getBitWidth() <= 64, "Cannot codegen for integers > 64 bits in width");

	const Helix::Integer val = Helix::Integer(integerLiteralValue.getZExtValue());
	const Helix::Type* ty = this->ConvertType(integerLiteral->getType());

	return Helix::ConstantInt::Create(ty, val);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Helix::Value* CodeGenerator::DoBinOp(clang::BinaryOperator* binOp)
{
	using namespace Helix;

	if (binOp->getOpcode() == clang::BO_Assign) {
		return this->DoAssignment(binOp);
	}

	Value* lhs = this->DoExpr(binOp->getLHS());
	Value* rhs = this->DoExpr(binOp->getRHS());

	HLIR::Opcode opc = HLIR::Undefined;

	switch (binOp->getOpcode()) {
	case clang::BO_Add: opc = HLIR::IAdd; break;
	case clang::BO_Sub: opc = HLIR::ISub; break;
	case clang::BO_Mul: opc = HLIR::IMul; break;
	case clang::BO_LT:  opc = HLIR::ICmp_Lt; break;
	case clang::BO_GT:  opc = HLIR::ICmp_Gt; break;
	case clang::BO_LE:  opc = HLIR::ICmp_Lte; break;
	case clang::BO_GE:  opc = HLIR::ICmp_Gte; break;
	case clang::BO_EQ:  opc = HLIR::ICmp_Eq; break;
	case clang::BO_NE:  opc = HLIR::ICmp_Neq; break;
	case clang::BO_And: opc = HLIR::And; break;
	case clang::BO_Or:  opc = HLIR::Or; break;
	case clang::BO_Xor: opc = HLIR::Xor; break;
	case clang::BO_Rem: {
		if (binOp->getType()->hasUnsignedIntegerRepresentation()) {
			opc = HLIR::IURem;
		} else {
			opc = HLIR::ISRem;
		}
		break;
	}

	case clang::BO_Div: {
		if (binOp->getType()->hasUnsignedIntegerRepresentation()) {
			opc = HLIR::IUDiv;
		} else {
			opc = HLIR::ISDiv;
		}
		break;
	}
	default:
		frontend_unimplemented_at("Unsupported binary expression", binOp->getOperatorLoc());
		break;
	}

	const Type* resultType = this->ConvertType(binOp->getType());

	VirtualRegisterName* result = VirtualRegisterName::Create(resultType);
	Instruction* insn = nullptr;

	if (Helix::HLIR::IsCompare(opc)) {
		insn = Helix::CreateCompare(opc, lhs, rhs, result);
	} else {
		insn = Helix::CreateBinOp(opc, lhs, rhs, result);
	}

	frontend_assert(insn, "Null binary operator instruction");

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
	case clang::Stmt::InitListExprClass:
		return DoInitListExpr(clang::cast<clang::InitListExpr>(expr));

	case clang::Stmt::CharacterLiteralClass:
		return DoCharacterLiteral(clang::cast<clang::CharacterLiteral>(expr));

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
			return DoSizeOf(uett->getTypeOfArgument());
		default:
			frontend_unimplemented_at("unknown unary operator or type trait", uett->getExprLoc());
			break;
		}

		break;
	}

	case clang::Stmt::ConstantExprClass: {
		clang::ConstantExpr* cexpr = clang::cast<clang::ConstantExpr>(expr);

		helix_assert(cexpr->getSubExpr(), "null subexpr");

		return this->DoExpr(cexpr->getSubExpr());
	}

	case clang::Stmt::DeclRefExprClass: {
		clang::DeclRefExpr* declRefExpr = clang::cast<clang::DeclRefExpr>(expr);
		clang::ValueDecl*   valueDecl   = declRefExpr->getDecl();

		switch (valueDecl->getKind()) {
		case clang::ValueDecl::EnumConstant: {
			clang::EnumConstantDecl* enumConstantDecl = clang::cast<clang::EnumConstantDecl>(valueDecl);
			
			const Helix::Integer integralValue = [this, enumConstantDecl]() {
				if (enumConstantDecl->getInitExpr()) {
					return this->EvaluteConstantIntegralExpression(enumConstantDecl->getInitExpr());
				}
				else {
					return enumConstantDecl->getInitVal().getZExtValue();
				}
			}();

			return Helix::ConstantInt::Create(Helix::BuiltinTypes::GetInt32(), integralValue);
		}

		default:
			frontend_unimplemented_at("unknown DeclRefExpr here", expr->getExprLoc());
			break;
		}

		break;
	}
	
	default:
		frontend_unimplemented_at(
			fmt::format("Cannot codegen for unsupported expression type '{}'", expr->getStmtClassName()),
			expr->getExprLoc()
		);
		break;
	}
	}
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CodeGenerator::DoFunctionDecl(clang::FunctionDecl* functionDecl)
{
	HELIX_PROFILE_ZONE;

	using namespace Helix;

	Function::ParamList          parameterValues;
	FunctionType::ParametersList parameterTypes;

	for (clang::ParmVarDecl* param : functionDecl->parameters()) {
		const Type*          ty            = this->ConvertType(param->getType());

		parameterValues.push_back(VirtualRegisterName::Create(ty));
		parameterTypes.push_back(ty);
	}

	const Type* returnType = this->ConvertType(functionDecl->getReturnType());
	const FunctionType* functionType = FunctionType::Create(returnType, parameterTypes);

	m_CurrentFunction = Function::Create(functionType, functionDecl->getNameAsString(), parameterValues);
	m_ValueMap.clear();

	m_FunctionDecls.insert({
		functionDecl,
		m_CurrentFunction
	});

	// Add the new function to the list of functions that we've generated code for in
	// this translation unit.
	m_Module->RegisterFunction(m_CurrentFunction);

	if (!functionDecl->hasBody()) {
		m_BasicBlockIterator.invalidate();
		m_InstructionIterator.invalidate();
		m_CurrentFunction = nullptr;
		return;
	}

	// Reset the basic block insert point so that the next basic block will be created
	// at the start of the new function.
	//
	// Create a new basic block so that we can start inserting instructions straight
	// away, without having to explicitly create one later.
	m_BasicBlockIterator = m_CurrentFunction->begin();
	EmitBasicBlock(CreateBasicBlock());

	for (size_t i = 0; i < parameterValues.size(); ++i) {
		const Type* ty = parameterValues[i]->GetType();
		VirtualRegisterName* addr = VirtualRegisterName::Create(BuiltinTypes::GetPointer());

		this->EmitInsn(Helix::CreateStackAlloc(addr, ty));
		this->EmitInsn(Helix::CreateStore(parameterValues[i], addr));

		m_ValueMap.insert({*(functionDecl->param_begin() + i), addr });
	}

	this->DoStmt(functionDecl->getBody());

	bool tryInjectRet = false;

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
		else {
			// If we can't delete the BB (because it might have outstanding references,
			// such as being jumped to) then try inject a ret.
			// This might happen, for void functions where a branching statement like an
			// 'if' has created a tail block, but nothing occurs after the if statement
			// so its just left empty, but used as a jump target to escape the ret.
			if (m_BasicBlockIterator->IsEmpty())
				tryInjectRet = true;
		}
	}
	else {
		tryInjectRet = true;
	}
	
	if (tryInjectRet) {
		frontend_assert(m_CurrentFunction->GetCountBlocks() > 0, "Function does not contain any blocks");

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

	g_GlobalASTContext = &ctx;

	// #FIXME: Kind of a hack (instead of handling all initialisation in the CodeGenerator
	//         constructor) so that it can use the g_GlobalASTContext, which is only valid now...
	m_CodeGen.Initialise();

	m_CodeGen.CodeGenTranslationUnit(ctx.getTranslationUnitDecl());
	g_GlobalASTContext = nullptr;

	g_TranslationUnit = m_CodeGen.GetModule();
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

void Helix::Frontend::Clang::Initialise()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void PrintVersion(llvm::raw_ostream& output_stream)
{
	output_stream << HELIX_APP_NAME " version " HELIX_VERSION "\n";
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Helix::Module* Helix::Frontend::Clang::Run(int argc, const char** argv)
{
	HELIX_PROFILE_ZONE;

	llvm::cl::SetVersionPrinter(PrintVersion);
	llvm::cl::HideUnrelatedOptions(HelixGeneralOptionsCategory);

	llvm::cl::ParseCommandLineOptions(argc, argv);

	{
		HELIX_PROFILE_ZONE;
		
		clang::tooling::FixedCompilationDatabase compilationDatabase(".", std::vector<std::string>());

		std::vector<std::string> sourceFiles;
		sourceFiles.reserve(Options::GetCountSourceFiles());

		for (size_t i = 0; i < Options::GetCountSourceFiles(); ++i) {
			sourceFiles.push_back(Options::GetSourceFile(i));
		}

		clang::tooling::ClangTool tool(
			compilationDatabase,
			sourceFiles
		);

		std::string defines = "";

		for (size_t i = 0; i < Options::GetCountPP_Definess(); ++i) {
			const std::string& define = Options::GetPP_Defines(i);

			defines += "-D" + define + " ";
		}

		tool.appendArgumentsAdjuster(clang::tooling::getInsertArgumentAdjuster(
			{
				"--target=armv7-pc-linux-eabi",
				"-nostdlib",

				// This might not be nessesary as it doesn't seem to be finding
				// std headers anyway without it, but it matches -nostdlib
				// and it can't hurt to make sure
				"-nostdinc",
				defines,

			#if defined(CONFIG_LIBC_INCLUDE_DIRECTORY)
				"-I" CONFIG_LIBC_INCLUDE_DIRECTORY
			#endif
			},
			clang::tooling::ArgumentInsertPosition::BEGIN
		));

		for (size_t i = 0; i < Options::GetCountEnabledLogs(); ++i) {
			const std::string& opt = Options::GetEnabledLog(i);

			if (opt == "all") {
				LogRegister::set_all_log_levels(spdlog::level::trace);
				break;
			}
			else {
				LogRegister::set_log_level(opt.c_str(), spdlog::level::trace);
			}
		}

		const int status = tool.run(clang::tooling::newFrontendActionFactory<ParserAction>().get());

		if (status > 0)
			return nullptr;
	}

	return g_TranslationUnit;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Helix::Frontend::Clang::Shutdown()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
