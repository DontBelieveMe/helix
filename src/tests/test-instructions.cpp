#include "catch.hpp"
#include "../instructions.h"
#include "../function.h"

using namespace Helix;

/*********************************************************************************************************************/

TEST_CASE("Operand flags", "[Instruction]")
{
	ConstantInt* a = ConstantInt::Create(BuiltinTypes::GetInt32(), 5);
	ConstantInt* b = ConstantInt::Create(BuiltinTypes::GetInt32(), 10);
	VirtualRegisterName* reg = VirtualRegisterName::Create(BuiltinTypes::GetInt32());

	Function* fnWithNoParameters = Function::Create(
		Helix::FunctionType::Create(BuiltinTypes::GetVoidType(), {}),
		"noparams",
		{}
	);

	Function* fnWithParameters = Function::Create(
		Helix::FunctionType::Create(BuiltinTypes::GetVoidType(), { BuiltinTypes::GetInt32() }),
		"params",
		{ reg }
	);

	BasicBlock* bb0 = BasicBlock::Create();
	BasicBlock* bb1 = BasicBlock::Create();

	SECTION("kInsn_IAdd") {
		BinOpInsn* binop = Helix::CreateBinOp(kInsn_IAdd, a, b, reg);

		REQUIRE(binop->OperandHasFlags(0, Instruction::OP_READ));
		REQUIRE(binop->OperandHasFlags(1, Instruction::OP_READ));
		REQUIRE(binop->OperandHasFlags(2, Instruction::OP_WRITE));
	}

	SECTION("kInsn_And") {
		BinOpInsn* binop = Helix::CreateBinOp(kInsn_And, a, b, reg);

		REQUIRE(binop->OperandHasFlags(0, Instruction::OP_READ));
		REQUIRE(binop->OperandHasFlags(1, Instruction::OP_READ));
		REQUIRE(binop->OperandHasFlags(2, Instruction::OP_WRITE));
	}

	SECTION("kInsn_Compare") {
		CompareInsn* compare = Helix::CreateCompare(kInsn_ICmp_Gt, a, b, reg);

		REQUIRE(compare->GetOperandFlags(0) == Instruction::OP_READ);
		REQUIRE(compare->GetOperandFlags(1) == Instruction::OP_READ);
		REQUIRE(compare->GetOperandFlags(2) == Instruction::OP_WRITE);
		REQUIRE(compare->GetOperandFlags(3) == Instruction::OP_NONE);
	}

	SECTION("kInsn_Ret (With Return Value)") {
		RetInsn* ret = Helix::CreateRet(a);

		REQUIRE(ret->OperandHasFlags(0, Instruction::OP_READ));
	}

	SECTION("kInsn_Ret (Without Return Value)") {
		RetInsn* ret = Helix::CreateRet();

		REQUIRE(ret->GetOperandFlags(0) == Instruction::OP_NONE);
	}

	SECTION("kInsn_StackAlloc") {
		StackAllocInsn* stack_alloc = Helix::CreateStackAlloc(reg, BuiltinTypes::GetInt32());

		REQUIRE(stack_alloc->GetOperandFlags(0) == Instruction::OP_WRITE);
		REQUIRE(stack_alloc->GetOperandFlags(1) == Instruction::OP_NONE);
		REQUIRE(stack_alloc->GetOperandFlags(1024) == Instruction::OP_NONE);
	}

	SECTION("kInsn_Cbr") {
		ConditionalBranchInsn* cbr = Helix::CreateConditionalBranch(bb0, bb1, reg);

		REQUIRE(cbr->GetOperandFlags(0) == Instruction::OP_READ);
		REQUIRE(cbr->GetOperandFlags(1) == Instruction::OP_READ);
		REQUIRE(cbr->GetOperandFlags(2) == Instruction::OP_READ);
		REQUIRE(cbr->GetOperandFlags(3) == Instruction::OP_NONE);
	}

	SECTION("kInsn_Call (With Parameters)") {
		CallInsn* call = Helix::CreateCall(fnWithNoParameters, {});

		REQUIRE(call->OperandHasFlags(0, Instruction::OP_WRITE));
		REQUIRE(call->OperandHasFlags(1, Instruction::OP_READ));
		
		REQUIRE(call->GetOperandFlags(2) == Instruction::OP_NONE);
		REQUIRE(call->GetOperandFlags(3) == Instruction::OP_NONE);
	}

	SECTION("kInsn_Call (With Parameters)") {
		CallInsn* call = Helix::CreateCall(fnWithParameters, { a });

		REQUIRE(call->OperandHasFlags(0, Instruction::OP_WRITE));
		REQUIRE(call->OperandHasFlags(1, Instruction::OP_READ));

		REQUIRE(call->GetOperandFlags(2) == Instruction::OP_READ);
		REQUIRE(call->GetOperandFlags(3) == Instruction::OP_NONE);
	}
}

/*********************************************************************************************************************/
