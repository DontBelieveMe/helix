#include "frontend.h"

#include <stdio.h>

// Can't do anything about the compiler warnings in clangs/LLVMs
// own headers so just ignore them (they do nothing but clog up
// our own build output).
// This of course means that only clang & LLVM headers should
// be included in here, everything else should give off warnings!
//
// ****************** IGNORE WARNINGS START ******************
#pragma warning(push, 0) 

#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

#include <llvm/Support/CommandLine.h>

#pragma warning(pop)
// ****************** IGNORE WARNINGS END ******************

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static llvm::cl::OptionCategory Category("helx options");
static llvm::cl::extrahelp      CommonHelp(clang::tooling::CommonOptionsParser::HelpMessage);
static llvm::cl::extrahelp      MoreHelp("\nHelix C/C++ Compiler...\n");

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ParserASTConsumer : public clang::ASTConsumer
{
public:
	virtual void HandleTranslationUnit(clang::ASTContext& ctx);
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ParserAction : public clang::ASTFrontendAction
{
public:
	virtual std::unique_ptr<clang::ASTConsumer>
		CreateASTConsumer(clang::CompilerInstance& ci, clang::StringRef inFile);
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ParserASTConsumer::HandleTranslationUnit(clang::ASTContext& ctx)
{
	clang::TranslationUnitDecl* tu = ctx.getTranslationUnitDecl();

	for (auto it = tu->decls_begin(); it != tu->decls_end(); it++) {
		if(it->getKind() == clang::Decl::Kind::Function) {
			const clang::FunctionDecl* fn = it->getAsFunction();
			const std::string name = fn->getNameAsString();

			printf("function '%s'\n", name.c_str());

			clang::QualType returnType = fn->getReturnType().getCanonicalType();

			const std::string returnTypeString = returnType.getAsString();

			printf("\ttype: '%s'\n", returnTypeString.c_str());

			for (clang::ParmVarDecl* param : fn->parameters()) {
				const std::string pname = param->getNameAsString();
				const std::string ptype = param->getType().getCanonicalType().getAsString();

				printf("\tparam '%s' ('%s')\n", pname.c_str(), ptype.c_str());
			}
		}
		else if (it->getKind() == clang::Decl::Kind::Record)
		{
			clang::Decl* decl = *it;
			const clang::RecordDecl* record = llvm::dyn_cast<clang::RecordDecl>(decl);
			assert(record);
			const std::string name = record->getNameAsString();
			printf("struct '%s'\n", name.c_str());

			for (clang::FieldDecl* field : record->fields())
			{
				const std::string fname = field->getNameAsString();
				const std::string ftype = field->getType().getCanonicalType().getAsString();

				printf("\tfield '%s' ('%s')\n", fname.c_str(), ftype.c_str());
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<clang::ASTConsumer> ParserAction::CreateASTConsumer(clang::CompilerInstance& ci, clang::StringRef inFile)
{
	(void) ci;
	(void) inFile;

	return std::make_unique<ParserASTConsumer>();
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
