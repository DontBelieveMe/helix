#include "catch.hpp"
#include "../function.h"

using namespace Helix;

/*********************************************************************************************************************/

TEST_CASE("Creating a empty function", "[Function]")
{
	const FunctionType* type = FunctionType::Create(BuiltinTypes::GetVoidType(), {});

	Function* fn = Function::Create(type, "main", {});
	REQUIRE(fn->GetName() == "main");
	REQUIRE(fn->GetReturnType() == BuiltinTypes::GetVoidType());
}

/*********************************************************************************************************************/

TEST_CASE("Creating a function, inserting one basic block at end", "[Function]")
{
	const FunctionType* type = FunctionType::Create(BuiltinTypes::GetVoidType(), {});

	Function* fn = Function::Create(type, "main", {});
	
	BasicBlock* bb = BasicBlock::Create();
	fn->InsertBefore(fn->end(), bb);

	REQUIRE(fn->GetReturnType() == BuiltinTypes::GetVoidType());

	for (const BasicBlock& v : *fn) {
		REQUIRE(&v == bb);
	}
}

/*********************************************************************************************************************/
