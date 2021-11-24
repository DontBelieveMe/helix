#include "frontend.h"
#include "helix.h"
#include "core.h"

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

class CodeGenerator : public clang::RecursiveASTVisitor<CodeGenerator>
{
public:
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

	Helix::Value* DoExpr(clang::Expr* expr);
	Helix::Value* DoIntegerLiteral(clang::IntegerLiteral* integerLiteral);
	Helix::Value* DoBinOp(clang::BinaryOperator* binOp);
	Helix::Value* DoImplicitCastExpr(clang::ImplicitCastExpr* implicitCastExpr);
	Helix::Value* DoDeclRefExpr(clang::DeclRefExpr* declRefExpr);
	Helix::Value* DoParenExpr(clang::ParenExpr* parenExpr);
	Helix::Value* DoAssignment(clang::BinaryOperator* binOp);

private:
	std::vector<Helix::Function*>    m_Functions;
	Helix::Function::block_iterator  m_BasicBlockIterator;
	Helix::BasicBlock::insn_iterator m_InstructionIterator;
	Helix::Function*                 m_CurrentFunction = nullptr;

	std::unordered_map<clang::ValueDecl*, Helix::VirtualRegisterName*> m_ValueMap;
};

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

	this->EmitBasicBlock(body);
	m_InstructionIterator = body->begin();
	this->DoStmt(whileStmt->getBody());

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

	case clang::Stmt::BinaryOperatorClass:
		this->DoBinOp(clang::dyn_cast<clang::BinaryOperator>(stmt));
		break;

	case clang::Stmt::IfStmtClass:
		this->DoIfStmt(clang::dyn_cast<clang::IfStmt>(stmt));
		break;

	case clang::Stmt::WhileStmtClass:
		this->DoWhileStmt(clang::dyn_cast<clang::WhileStmt>(stmt));
		break;
	
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
	helix_assert(declStmt->isSingleDecl());
	this->DoDecl(declStmt->getSingleDecl());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CodeGenerator::DoVarDecl(clang::VarDecl* varDecl)
{
	using namespace Helix;

	// Create a register used to store the address of this variable on the stack...
	VirtualRegisterName* variableAddressRegister = VirtualRegisterName::Create(BuiltinTypes::GetPointer());

	// ... and then actually create the instruction to allocate space for the variable.
	//
	//              #FIXME(bwilks): This needs to pass some type information so that
	//                              it knows how much space to allocate :)
	EmitInsn(Helix::CreateStackAlloc(variableAddressRegister, BuiltinTypes::GetInt32(), 1));

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
	clang::DeclRefExpr* lhsExpression = clang::dyn_cast<clang::DeclRefExpr>(binOp->getLHS());

	Helix::VirtualRegisterName* variableAddress = this->FindValueForDecl(lhsExpression->getDecl());
	Helix::Value* rhs = this->DoExpr(binOp->getRHS());

	EmitInsn(Helix::CreateStore(rhs, variableAddress));

	return rhs;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Helix::Value* CodeGenerator::DoParenExpr(clang::ParenExpr* parenExpr)
{
	return this->DoExpr(parenExpr->getSubExpr());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Helix::Value* CodeGenerator::DoDeclRefExpr(clang::DeclRefExpr* declRefExpr)
{
	// Contains the pointer to the value on the stack
	Helix::VirtualRegisterName* stackAddressRegister = FindValueForDecl(declRefExpr->getDecl());
	helix_assert(stackAddressRegister, "Value not generated for declaration");

	Helix::VirtualRegisterName* value = Helix::VirtualRegisterName::Create(Helix::BuiltinTypes::GetInt32());
	EmitInsn(Helix::CreateLoad(stackAddressRegister, value));
	return value;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Helix::Value* CodeGenerator::DoIntegerLiteral(clang::IntegerLiteral* integerLiteral)
{
	const llvm::APInt integerLiteralValue = integerLiteral->getValue();

	helix_assert(integerLiteralValue.getBitWidth() <= 64, "Cannot codegen for integers > 64 bits in width");

	const Helix::Integer val = Helix::Integer(integerLiteralValue.getZExtValue());
	const Helix::Type* ty = Helix::BuiltinTypes::GetInt32();

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

	Helix::VirtualRegisterName* result = Helix::VirtualRegisterName::Create(Helix::BuiltinTypes::GetInt32());
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
	helix_warn("ImplicitCastExpr ignored");
	return this->DoExpr(implicitCastExpr->getSubExpr());
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
	case clang::Stmt::DeclRefExprClass:
		return DoDeclRefExpr(clang::dyn_cast<clang::DeclRefExpr>(expr));
	case clang::Stmt::ParenExprClass:
		return DoParenExpr(clang::dyn_cast<clang::ParenExpr>(expr));

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
	using namespace Helix;

	// Create the function
	m_CurrentFunction = Function::Create(functionDecl->getNameAsString());

	// Reset the basic block insert point so that the next basic block will be created
	// at the start of the new function.
	//
	// Create a new basic block so that we can start inserting instructions straight
	// away, without having to explicitly create one later.
	m_BasicBlockIterator = m_CurrentFunction->begin();
	EmitBasicBlock(CreateBasicBlock());

	// And clear the value map, since variables don't persist accross functions.
	m_ValueMap.clear();

	// Add the new function to the list of functions that we've generated code for in
	// this translation unit.
	m_Functions.push_back(m_CurrentFunction);

	if (functionDecl->hasBody())
		this->DoStmt(functionDecl->getBody());

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
	(void) ci; (void) inFile;

	return std::make_unique<CodeGenerator_ASTConsumer>();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Helix::Frontend::Initialise()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Helix::Frontend::Run(int argc, const char** argv)
{
	auto expectedOptionsParser = clang::tooling::CommonOptionsParser::create(
		argc, argv, Category
	);

	if (!expectedOptionsParser) {
		llvm::errs() << expectedOptionsParser.takeError();
		return;
	}

	clang::tooling::CommonOptionsParser& optionsParser = expectedOptionsParser.get();

	clang::tooling::ClangTool tool(
		optionsParser.getCompilations(),
		optionsParser.getSourcePathList()
	);

	tool.run(clang::tooling::newFrontendActionFactory<ParserAction>().get());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Helix::Frontend::Shutdown()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
