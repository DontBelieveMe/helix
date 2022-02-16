/**
 * @file peephole-generic.cpp
 * @author Barney Wilks
 */

#include "peephole-generic.h"
#include "function.h"
#include "instructions.h"
#include "ir-helpers.h"

using namespace Helix;

#pragma optimize("", off)

void PeepholeGeneric::Execute(Function* fn, const PassRunInformation& info)
{
	for (BasicBlock& bb : fn->blocks()) {
		BasicBlock::iterator input = bb.begin();

		while (input != bb.end()) {
			input = DoInstruction(input, nullptr);
		}
	}
}

BasicBlock::iterator PeepholeGeneric::DoInstruction(BasicBlock::iterator input, bool* bFlagChanges)
{
	Instruction* insn = &(*input);

	if (input->GetOpcode() == HLIR::IMul) {
		return DoIMul((BinOpInsn*)insn, bFlagChanges);
	}

	return input++;
}

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

			return next;
		}
	}

	return IR::GetNext(imul);
}