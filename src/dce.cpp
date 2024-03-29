/**
 * @file dce.cpp
 * @author Barney Wilks
 */

#include "dce.h"
#include "function.h"
#include "ir-helpers.h"

using namespace Helix;

static bool CanBeTriviallyDead(const Instruction& insn)
{
	if (HLIR::IsBranch((HLIR::Opcode) insn.GetOpcode()))
		return false;

	return true;
}

void DCE::Execute(Function* fn, const PassRunInformation&)
{
	std::vector<Instruction*> KillList;

	for (BasicBlock& bb : fn->blocks()) {
		for (Instruction& insn : bb) {
			if (!CanBeTriviallyDead(insn))
				continue;

			for (size_t op_index = 0; op_index < insn.GetCountOperands(); ++op_index) {
				Value* op = insn.GetOperand(op_index);

				if (insn.OperandHasFlags(op_index, Instruction::OP_WRITE)) {
					if (IR::GetCountReadUsers(op) == 0) {
						KillList.push_back(&insn);
					}
				}
			}
		}
	}

	for (Instruction* insn : KillList)
		insn->DeleteFromParent();
}