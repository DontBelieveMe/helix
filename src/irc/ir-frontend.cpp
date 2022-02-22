/* Internal Project Includes */
#include "ir-frontend.h"

/* Nothing really to do about warnings in external/generated
 * files so suppress them all, muhahahaha (please don't
 * put any other headers in here) */

/* SUPPRESS WARNINGS START */
/*********************************************************************************************************************/

#pragma warning(push, 0) 

#include "antlr4-runtime.h"

/* Generated */
#include "IRLexer.h"
#include "IRParser.h"
#include "IRBaseVisitor.h"

#pragma warning(pop)

/* SUPPRESS WARNINGS END */
/*********************************************************************************************************************/

/* Helix Core Includes */
#include "../module.h"

/* C++ Standard Library Includes  */
#include <vector>
#include <iostream> /* :-(, cries in iostream */
#include <string>

using namespace Helix;

class FrontendParser : public Helix::Frontend::IR::IRBaseVisitor {
public:
	FrontendParser(Module* pModule);

private:
	Module* m_Module = nullptr;
};

FrontendParser::FrontendParser(Module* pModule)
	: m_Module(pModule)
{ }

static Module* CompileFile(const std::string& filepath)
{
	using namespace Helix::Frontend::IR;
	using namespace antlr4;

	// Open a stream for the source file
	std::ifstream stream;
	stream.open(filepath);

	// Setup all the ANTLR classes
	ANTLRInputStream input(stream);
	IRLexer lexer(&input);
	CommonTokenStream tokens(&lexer);
	IRParser parser(&tokens);

	// Actually kickoff and parse the module

	Module* result = Helix::CreateModule(filepath);

	IRParser::ProgramContext *tree = parser.module();
	FrontendParser myParser(result);
	myParser.visitDecl(tree);

	return result;
}

void Helix::Frontend::IR::Initialise()
{
}

Module* Helix::Frontend::IR::Run(int argc, const char** argv)
{
	std::string file;

	for (int i = 0; i < argc; ++i) {
		if (argv[i][0] != '-')
			file = argv[i];
	}

	if (file.empty())
		return nullptr;

	return CompileFile(file);
}

void Helix::Frontend::IR::Shutdown()
{
}
