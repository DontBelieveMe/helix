/**
 * @file peephole-generic.cpp
 * @author Barney Wilks
 */

#include "peephole-generic.h"
#include "function.h"
#include "instructions.h"
#include "ir-helpers.h"

using namespace Helix;

/*********************************************************************************************************************/

void PeepholeGeneric::Execute(Function* fn, const PassRunInformation&)
{
	size_t nPasses = 0;

	for (;;) {
		bool bFlagChanges = false;

		for (BasicBlock& bb : fn->blocks()) {
			BasicBlock::iterator input = bb.begin();

			while (input != bb.end()) {
				input = DoInstruction(input, &bFlagChanges);
			}
		}

		nPasses++;

		if (!bFlagChanges)
			break;
	}

	helix_debug(logs::peepholegeneric, "Generic peephole optimiser executed in {} passes", nPasses);
}

/*********************************************************************************************************************/

BasicBlock::iterator PeepholeGeneric::DoInstruction(BasicBlock::iterator input, bool* bFlagChanges)
{
	Instruction* insn = &(*input);

	if (HLIR::IsBinaryOp((HLIR::Opcode) input->GetOpcode())) {
		return DoGenericBinOp((BinOpInsn*)insn, bFlagChanges);
	}

	if (input->GetOpcode() == HLIR::IMul) {
		return DoIMul((BinOpInsn*)insn, bFlagChanges);
	}

	return input++;
}

/*********************************************************************************************************************/

static ConstantInt* FoldConstantBinaryOperation(HLIR::Opcode opc, ConstantInt* lhs, ConstantInt* rhs)
{
	helix_assert(lhs->GetType() == rhs->GetType(), "LHS and RHS types must be the same in order to fold binop :)");

	Helix::Integer result = 0;

	/* #FIXME: Handle overflow for differently sized types correctly :) */
	switch (opc) {
	case HLIR::IAdd:  result = lhs->GetIntegralValue() + rhs->GetIntegralValue(); break;
	case HLIR::ISub:  result = lhs->GetIntegralValue() - rhs->GetIntegralValue(); break;
	case HLIR::IMul:  result = lhs->GetIntegralValue() * rhs->GetIntegralValue(); break;

	/* #FIXME: Add support for signed/unsigned division */
	default:
		return nullptr;
	}

	return ConstantInt::Create(lhs->GetType(), result);
}

/*********************************************************************************************************************/

BasicBlock::iterator PeepholeGeneric::DoGenericBinOp(BinOpInsn* binop, bool* bFlagChanges)
{
	Value* lhs = binop->GetLHS();
	Value* rhs = binop->GetRHS();

	if (value_isa<ConstantInt>(lhs) && value_isa<ConstantInt>(rhs)) {
		ConstantInt* lhsIntegerValue = value_cast<ConstantInt>(lhs);
		ConstantInt* rhsIntegerValue = value_cast<ConstantInt>(rhs);

		ConstantInt* result = FoldConstantBinaryOperation((HLIR::Opcode) binop->GetOpcode(), lhsIntegerValue, rhsIntegerValue);

		if (result) {
			IR::ReplaceAllUsesWith(binop->GetResult(), result);

			BasicBlock::iterator next = IR::GetNext(binop);
			binop->DeleteFromParent();

			*bFlagChanges = true;

			return next;
		}
	}

	return IR::GetNext(binop);
}

/*********************************************************************************************************************/

BasicBlock::iterator PeepholeGeneric::DoIMul(BinOpInsn* imul, bool* bFlagChanges)
{
	Value* lhs = imul->GetLHS();
	Value* rhs = imul->GetRHS();

	if (ConstantInt* integer = value_cast<ConstantInt>(rhs)) {
		if (integer->GetIntegralValue() == 1) {
			Value* dst = imul->GetResult();
			IR::ReplaceAllUsesWith(dst, lhs);

			BasicBlock::iterator next = IR::GetNext(imul);
			imul->DeleteFromParent();

			*bFlagChanges = true;

			return next;
		}
	}

	return IR::GetNext(imul);
}

/*********************************************************************************************************************/
