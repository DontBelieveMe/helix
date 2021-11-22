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
	bool VisitReturnStmt(clang::ReturnStmt* returnStmt);
	bool VisitVarDecl(clang::VarDecl* varDecl);

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
	Helix::Value* DoExpr(clang::Expr* expr);
	Helix::Value* DoIntegerLiteral(clang::IntegerLiteral* integerLiteral);
	Helix::Value* DoBinOp(clang::BinaryOperator* binOp);
	Helix::Value* DoImplicitCastExpr(clang::ImplicitCastExpr* implicitCastExpr);
	Helix::Value* DoDeclRefExpr(clang::DeclRefExpr* declRefExpr);
	Helix::Value* DoParenExpr(clang::ParenExpr* parenExpr);

private:
	std::vector<Helix::Function*>    m_Functions;
	Helix::Function::block_iterator  m_BasicBlockIterator;
	Helix::BasicBlock::insn_iterator m_InstructionIterator;
	Helix::Function*                 m_CurrentFunction = nullptr;

	std::unordered_map<clang::ValueDecl*, Helix::VirtualRegisterName*> m_ValueMap;
};

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
	Helix::Value* lhs = this->DoExpr(binOp->getLHS());
	Helix::Value* rhs = this->DoExpr(binOp->getRHS());

	Helix::Opcode opc = Helix::kInsn_Undefined;

	switch (binOp->getOpcode()) {
	case clang::BO_Add: opc = Helix::kInsn_IAdd; break;
	case clang::BO_Sub: opc = Helix::kInsn_ISub; break;
	case clang::BO_Div: opc = Helix::kInsn_IDiv; break;
	case clang::BO_Mul: opc = Helix::kInsn_IMul; break;
	default:
		helix_unreachable("Unsupported binary expression");
		break;
	}

	Helix::VirtualRegisterName* result = Helix::VirtualRegisterName::Create(Helix::BuiltinTypes::GetInt32());
	EmitInsn(Helix::CreateBinOp(opc, lhs, rhs, result));
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

bool CodeGenerator::VisitVarDecl(clang::VarDecl* varDecl)
{
	using namespace Helix;

	// Create a register used to store the address of this variable on the stack...
	VirtualRegisterName* variableAddressRegister = VirtualRegisterName::Create(BuiltinTypes::GetPointer());

	// ... and then actually create the instruction to allocate space for the variable.
	//
	//              #FIXME(bwilks): This needs to pass some type information so that
	//                              it knows how much space to allocate :)
	EmitInsn(Helix::CreateStackAlloc(variableAddressRegister));

	if (varDecl->hasInit()) {
		// Evaluate the RHS assignment of this var decl and store it at the
		// address allocated for this stack var.
		Helix::Value* value = this->DoExpr(varDecl->getInit());
		EmitInsn(Helix::CreateStore(value, variableAddressRegister));
	}

	// Then create a mapping for this variable declaration so that later uses of this
	// variable know which address to load from.
	m_ValueMap.insert({varDecl, variableAddressRegister});

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CodeGenerator::VisitReturnStmt(clang::ReturnStmt* returnStmt)
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

	return true;
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
