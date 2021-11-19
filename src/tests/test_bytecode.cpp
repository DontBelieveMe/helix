#include "catch.hpp"

#include "..\instructions.h"
#include "..\basic_block.h"
#include "..\print.h"

using namespace Helix;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("Verify opcode categories", "[Bytecode]")
{
	REQUIRE(IsCompare(kInsn_FCmp));
	REQUIRE(IsCompare(kInsn_ICmp));
	REQUIRE(!IsCompare(kInsn_IAdd));
	REQUIRE(IsBinaryOp(kInsn_IAdd));

	REQUIRE(IsTerminator(kInsn_Cbr));
	REQUIRE(IsBranch(kInsn_Cbr));

	REQUIRE(IsTerminator(kInsn_Ret));
	REQUIRE(IsBranch(kInsn_Ret));

	REQUIRE(!IsTerminator(kInsn_Call));
	REQUIRE(IsBranch(kInsn_Call));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("Creating a floating point compare", "[Bytecode]")
{
	BasicBlock* bb = BasicBlock::Create();

	Value* lhs = ConstantInt::Create(BuiltinTypes::GetInt32(), 0);
	Value* rhs = ConstantInt::Create(BuiltinTypes::GetInt32(), 1);

	VirtualRegisterName* result = VirtualRegisterName::Create(BuiltinTypes::GetInt32(), "result");

	Instruction* fcmp = Helix::CreateCompare(kInsn_FCmp, lhs, rhs, result);

	REQUIRE(fcmp->GetOperand(0) == lhs);
	REQUIRE(fcmp->GetOperand(1) == rhs);
	REQUIRE(fcmp->GetOperand(2) == result);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("Creating a integral compare", "[Bytecode]")
{
	BasicBlock* bb = BasicBlock::Create();

	Value* v = ConstantInt::Create(BuiltinTypes::GetInt32(), 454);
	VirtualRegisterName* result = VirtualRegisterName::Create(BuiltinTypes::GetInt32(), "result");

	Instruction* fcmp = Helix::CreateCompare(kInsn_ICmp, v, v, result);

	REQUIRE(fcmp->GetOperand(0) == v);
	REQUIRE(fcmp->GetOperand(1) == v);
	REQUIRE(fcmp->GetOperand(2) == result);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("Creating a condtional branch", "[Bytecode]")
{
	BasicBlock* trueTarget = BasicBlock::Create("true");
	BasicBlock* falseTarget = BasicBlock::Create("false");

	Value* trueConstant = ConstantInt::Create(BuiltinTypes::GetInt32(), 1);

	BasicBlock* head = BasicBlock::Create("head");
	Instruction* cbr = Helix::CreateConditionalBranch(trueTarget, falseTarget, trueConstant);
	head->InsertBefore(head->end(), cbr);

	REQUIRE(value_cast<BlockBranchTarget>(cbr->GetOperand(0))->GetParent() == trueTarget);
	REQUIRE(value_cast<BlockBranchTarget>(cbr->GetOperand(1))->GetParent() == falseTarget);
	REQUIRE(cbr->GetOperand(2) == trueConstant);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("Creating an unconditional branch", "[Bytecode]")
{
	BasicBlock* head = BasicBlock::Create("head");
	BasicBlock* target = BasicBlock::Create("target");
	
	Instruction* br = Helix::CreateUnconditionalBranch(target);
	head->InsertBefore(head->end(), br);

	BlockBranchTarget* tmp = value_cast<BlockBranchTarget>(br->GetOperand(0));
	REQUIRE(tmp->GetParent() == target);
}

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

