#include "catch.hpp"
#include "..\instructions.h"

using namespace Helix;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("value_cast", "[Bytecode]")
{
	Value* value = VirtualRegisterName::Create("MyValue");

	ConstantInt*         ci   = value_cast<ConstantInt>(value);
	VirtualRegisterName* vreg = value_cast<VirtualRegisterName>(value);

	REQUIRE(!ci);
	REQUIRE(vreg);
	REQUIRE(strcmp(vreg->GetDebugName(), "MyValue") == 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("CreateBinOp Register/Register/Register", "[Bytecode]")
{
	VirtualRegisterName* lhs    = VirtualRegisterName::Create("left");
	VirtualRegisterName* rhs    = VirtualRegisterName::Create("right");
	VirtualRegisterName* output = VirtualRegisterName::Create("output");

	BinOp* add = Helix::CreateBinaryOp(
		kInsn_IAdd,
		lhs,
		rhs,
		output	
	);

	REQUIRE(add->GetOperand(0) == lhs);
	REQUIRE(add->GetOperand(1) == rhs);
	REQUIRE(add->GetOperand(2) == output);
	REQUIRE(add->GetCountOperands() == 3);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("CreateBinOp ConstantInt/ConstantInt/Register", "[Bytecode]")
{
	ConstantInt* lhs = ConstantInt::Create(BuiltinTypes::GetInt32(), 123);
	ConstantInt* rhs = ConstantInt::Create(BuiltinTypes::GetInt32(), 54);

	VirtualRegisterName* output = VirtualRegisterName::Create("output");

	BinOp* add = Helix::CreateBinaryOp(
		kInsn_IAdd,
		lhs,
		rhs,
		output	
	);

	REQUIRE(add->GetOperand(0) == lhs);
	REQUIRE(add->GetOperand(1) == rhs);
	REQUIRE(add->GetOperand(2) == output);
	REQUIRE(add->GetCountOperands() == 3);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

