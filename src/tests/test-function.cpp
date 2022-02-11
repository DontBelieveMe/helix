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

TEST_CASE("Appending a basic block to an empty function", "[Function]")
{
	const FunctionType* type = FunctionType::Create(BuiltinTypes::GetVoidType(), {});
	Function* fn = Function::Create(type, "main", {});

	BasicBlock* bb = BasicBlock::Create();
	fn->Append(bb);

	REQUIRE(fn->GetCountBlocks() == 1);
	REQUIRE(fn->GetHeadBlock() == bb);
	REQUIRE(fn->GetTailBlock() == bb);
}

/*********************************************************************************************************************/

TEST_CASE("Appending a basic block to a non empty function", "[Function]")
{
	const FunctionType* type = FunctionType::Create(BuiltinTypes::GetVoidType(), {});
	Function* fn = Function::Create(type, "main", {});

	BasicBlock* bb1 = BasicBlock::Create();
	BasicBlock* bb2 = BasicBlock::Create();
	fn->Append(bb1);
	fn->Append(bb2);

	REQUIRE(fn->GetCountBlocks() == 2);
	REQUIRE(fn->GetHeadBlock() == bb1);
	REQUIRE(fn->GetTailBlock() == bb2);
}


/*********************************************************************************************************************/
