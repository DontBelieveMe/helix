#include "catch.hpp"
#include "../function.h"

using namespace Helix;

TEST_CASE("Creating a empty function", "[Function]")
{
	Function* fn = Function::Create("main");
	REQUIRE(fn->GetName() == "main");
}

TEST_CASE("Creating a function, inserting one basic block at end", "[Function]")
{
	Function* fn = Function::Create("main");
	
	BasicBlock* bb = BasicBlock::Create();
	fn->InsertBefore(fn->end(), bb);

	for (const BasicBlock& v : *fn)
	{
		REQUIRE(&v == bb);
	}
}
