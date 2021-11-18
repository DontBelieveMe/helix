#include "catch.hpp"

#include "..\instructions.h"
#include "..\basic_block.h"
#include "..\print.h"

using namespace Helix;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("Inserting an instruction into a basic block", "[Bytecode]")
{
	const Type* i32 = BuiltinTypes::GetInt32();

	VirtualRegisterName* lhs = VirtualRegisterName::Create(i32, "lhs");
	VirtualRegisterName* rhs = VirtualRegisterName::Create(i32, "rhs");
	VirtualRegisterName* result = VirtualRegisterName::Create(i32, "temp");
	VirtualRegisterName* result2 = VirtualRegisterName::Create(i32, "temp2");

	Instruction* add = Helix::CreateBinOp(kInsn_IAdd, lhs, rhs, result);
	Instruction* sub = Helix::CreateBinOp(kInsn_ISub, result, rhs, result2);

	BasicBlock *bb = BasicBlock::Create("head");

	auto it = bb->InsertBefore(bb->end(), add);
	bb->InsertAfter(it, sub);

	Instruction* expected[] = { add, sub };

	bool matches = true;
	size_t index = 0;
	for (const Instruction& insn : *bb) {
		matches &= (&insn == expected[index]);
		index++;
	}
	REQUIRE(matches);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("value_cast", "[Bytecode]")
{
	Value* value = VirtualRegisterName::Create(BuiltinTypes::GetInt32(), "MyValue");

	ConstantInt*         ci   = value_cast<ConstantInt>(value);
	VirtualRegisterName* vreg = value_cast<VirtualRegisterName>(value);

	REQUIRE(!ci);
	REQUIRE(vreg);
	REQUIRE(strcmp(vreg->GetDebugName(), "MyValue") == 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("CreateBinOp Register/Register/Register", "[Bytecode]")
{
	const Type* i32 = BuiltinTypes::GetInt32();

	VirtualRegisterName* lhs    = VirtualRegisterName::Create(i32, "left");
	VirtualRegisterName* rhs    = VirtualRegisterName::Create(i32, "right");
	VirtualRegisterName* output = VirtualRegisterName::Create(i32, "output");

	BinOpInsn* add = Helix::CreateBinOp(
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

	VirtualRegisterName* output = VirtualRegisterName::Create(BuiltinTypes::GetInt32(), "output");

	BinOpInsn* add = Helix::CreateBinOp(
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

