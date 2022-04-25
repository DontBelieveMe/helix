#include "catch.hpp"

#include "../function.h"
#include "../module.h"

using namespace Helix;

/******************************************************************************/

TEST_CASE("Creating a empty function", "[Function]")
{
	const FunctionType* type
		= FunctionType::Create(BuiltinTypes::GetVoidType(), {});

	Function* fn = Function::Create(type, "main", {});

	REQUIRE(fn->GetName() == "main");
	REQUIRE(fn->GetReturnType() == BuiltinTypes::GetVoidType());
}

/******************************************************************************/

TEST_CASE("Creating a function, inserting one basic block at end", "[Function]")
{
	const FunctionType* type
		= FunctionType::Create(BuiltinTypes::GetVoidType(), {});

	Function* fn = Function::Create(type, "main", {});
	
	BasicBlock* bb = BasicBlock::Create();
	fn->InsertBefore(fn->end(), bb);

	REQUIRE(fn->GetReturnType() == BuiltinTypes::GetVoidType());

	for (const BasicBlock& v : *fn) {
		REQUIRE(&v == bb);
	}
}

/******************************************************************************/

TEST_CASE("Appending a basic block to an empty function", "[Function]")
{
	const FunctionType* type
		= FunctionType::Create(BuiltinTypes::GetVoidType(), {});

	Function* fn = Function::Create(type, "main", {});

	BasicBlock* bb = BasicBlock::Create();
	fn->Append(bb);

	REQUIRE(fn->GetCountBlocks() == 1);
	REQUIRE(fn->GetHeadBlock() == bb);
	REQUIRE(fn->GetTailBlock() == bb);
}

/******************************************************************************/

TEST_CASE("Appending a basic block to a non empty function", "[Function]")
{
	const FunctionType* type
		= FunctionType::Create(BuiltinTypes::GetVoidType(), {});

	Function* fn = Function::Create(type, "main", {});

	BasicBlock* bb1 = BasicBlock::Create();
	BasicBlock* bb2 = BasicBlock::Create();
	fn->Append(bb1);
	fn->Append(bb2);

	REQUIRE(fn->GetCountBlocks() == 2);
	REQUIRE(fn->GetHeadBlock() == bb1);
	REQUIRE(fn->GetTailBlock() == bb2);
}


/******************************************************************************/

TEST_CASE("Function::GetName", "[Function]")
{
	const FunctionType* type
		= FunctionType::Create(BuiltinTypes::GetVoidType(),
				{ });

	Function* fn = Function::Create(type, "main", { });

	REQUIRE(fn->GetName() == "main");
}

/******************************************************************************/

TEST_CASE("Function::GetParameter/GetCountParameters (One)", "[Function]")
{
	const FunctionType* type
		= FunctionType::Create(BuiltinTypes::GetVoidType(),
				{ BuiltinTypes::GetInt32() });

	VirtualRegisterName* vreg
		= VirtualRegisterName::Create(BuiltinTypes::GetInt32());

	Function* fn = Function::Create(type, "main", { vreg });

	REQUIRE(fn->GetParameter(0) == vreg);
	REQUIRE(fn->GetCountParameters() == 1);
}

/******************************************************************************/

TEST_CASE("Function::GetParameter/GetCountParameters (Out of Bounds)", "[Function]")
{
	const FunctionType* type
		= FunctionType::Create(BuiltinTypes::GetVoidType(), {});

	Function* fn = Function::Create(type, "main", { });

	REQUIRE(fn->GetParameter(23) == nullptr);
	REQUIRE(fn->GetCountParameters() == 0);
}

/******************************************************************************/

TEST_CASE("Function::HasBody", "[Function]")
{
	const FunctionType* type
		= FunctionType::Create(BuiltinTypes::GetVoidType(), {});

	Function* fn = Function::Create(type, "main", { });

	REQUIRE(!fn->HasBody());

	BasicBlock* bb = BasicBlock::Create();

	fn->Append(bb);

	REQUIRE(fn->HasBody());

	fn->Remove(fn->Where(bb));

	REQUIRE(!fn->HasBody());
}

/******************************************************************************/

TEST_CASE("Function::SetParent/GetParent", "[Function]")
{
	const FunctionType* type
		= FunctionType::Create(BuiltinTypes::GetVoidType(), {});

	Function* fn = Function::Create(type, "main", { });
	Module* mod = Helix::CreateModule("test");

	REQUIRE(fn->GetParent() == nullptr);

	fn->SetParent(mod);

	REQUIRE(fn->GetParent() == mod);

	fn->SetParent(nullptr);

	REQUIRE(fn->GetParent() == nullptr);
}

/******************************************************************************/

TEST_CASE("Function::IsVoidReturn (True)/GetReturnType", "[Function]")
{
	const FunctionType* type
		= FunctionType::Create(BuiltinTypes::GetVoidType(), {});

	Function* fn = Function::Create(type, "main", { });

	REQUIRE(fn->IsVoidReturn());
	REQUIRE(fn->GetReturnType() == BuiltinTypes::GetVoidType());

	const FunctionType* new_type
		= FunctionType::Create(BuiltinTypes::GetPointer(), {});

	fn->SetType(new_type);

	REQUIRE(!fn->IsVoidReturn());
	REQUIRE(fn->GetReturnType() == BuiltinTypes::GetPointer());
}

/******************************************************************************/

TEST_CASE("Function::IsVoidReturn (False)/GetReturnType", "[Function]")
{
	const FunctionType* type
		= FunctionType::Create(BuiltinTypes::GetInt32(), {});

	Function* fn = Function::Create(type, "main", { });

	REQUIRE(!fn->IsVoidReturn());
	REQUIRE(fn->GetReturnType() == BuiltinTypes::GetInt32());

	const FunctionType* new_type
		= FunctionType::Create(BuiltinTypes::GetVoidType(), {});

	fn->SetType(new_type);

	REQUIRE(fn->IsVoidReturn());
	REQUIRE(fn->GetReturnType() == BuiltinTypes::GetVoidType());
}

/******************************************************************************/

TEST_CASE("Helix::is_function", "[Function]")
{
	const FunctionType* type
		= FunctionType::Create(BuiltinTypes::GetInt32(), {});

	Function* fn = Function::Create(type, "main", { });

	VirtualRegisterName* vreg
		= VirtualRegisterName::Create(BuiltinTypes::GetInt32());

	REQUIRE(is_function(fn));
	REQUIRE(!is_function(vreg));
}


/******************************************************************************/


