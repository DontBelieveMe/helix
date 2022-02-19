/**
 * @file ir-helpers.cpp
 * @author Barney Wilks
 *
 * Implementation of ir-helpers.h
 */

/* Internal Project Includes */
#include "ir-helpers.h"

/* Standard Library Includes */
#include <vector>

using namespace Helix;

/*********************************************************************************************************************/

void IR::ReplaceAllUsesWith(Value* oldValue, Value* newValue)
{
	std::vector<Use> worklist;

	for (auto it = oldValue->uses_begin(); it != oldValue->uses_end(); ++it) {
		worklist.push_back(*it);
	}

	for (Use& use : worklist) {
		use.ReplaceWith(newValue);
	}
}

/*********************************************************************************************************************/

void IR::ReplaceInstructionAndPreserveOriginal(Instruction* a, Instruction* b)
{
	BasicBlock* parent = a->GetParent();
	parent->Replace(a, b);
}

/*********************************************************************************************************************/

void IR::ReplaceInstructionAndDestroyOriginal(Instruction* a, Instruction* b)
{
	BasicBlock* parent = a->GetParent();
	parent->Replace(a, b);

	IR::DestroyInstruction(a);
}

/*********************************************************************************************************************/

void IR::InsertAfter(Instruction* a, Instruction* b)
{
	BasicBlock* bb = a->GetParent();
	bb->InsertAfter(bb->Where(a), b);
}

/*********************************************************************************************************************/

void IR::InsertBefore(Instruction* a, Instruction* b)
{
	BasicBlock* bb = a->GetParent();
	bb->InsertBefore(bb->Where(a), b);
}

/*********************************************************************************************************************/

bool IR::TryGetSingleUser(Instruction* base, Value* v, Use* outUse)
{
	if (v->GetCountUses() != 2)
		return false;

	for (const Use& use : v->uses()) {
		if (use.GetInstruction() == base)
			continue;

		*outUse = use;
		return true;
	}

	return false;
}

/*********************************************************************************************************************/

void IR::DestroyInstruction(Instruction* insn)
{
	if (!insn)
		return;

	BasicBlock* parent = insn->GetParent();

	if (parent) {
		parent->Remove(parent->Where(insn));
	}

	insn->Clear();

	delete insn;
}

/*********************************************************************************************************************/

size_t IR::GetCountReadUsers(Value* v)
{
	size_t count = 0;

	for (const Use& use : v->uses()) {
		Instruction* insn = use.GetInstruction();

		if (insn->OperandHasFlags(use.GetOperandIndex(), Instruction::OP_READ))
			count++;
	}

	return count;
}

/*********************************************************************************************************************/

size_t IR::GetCountWriteUsers(Value* v)
{
	size_t count = 0;

	for (const Use& use : v->uses()) {
		Instruction* insn = use.GetInstruction();

		if (insn->OperandHasFlags(use.GetOperandIndex(), Instruction::OP_WRITE))
			count++;
	}

	return count;
}


/*********************************************************************************************************************/
