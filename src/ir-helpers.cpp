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

	a->Clear();
	Helix::DestroyInstruction(a);
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
