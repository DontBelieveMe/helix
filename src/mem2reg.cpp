/**
 * @file mem2reg.cpp
 * @author Barney Wilks
 */

/* Internal Project Includes */
#include "mem2reg.h"
#include "ir-helpers.h"
#include "function.h"

using namespace Helix;

/*********************************************************************************************************************/

void Mem2Reg::Execute(Function* fn, const PassRunInformation&)
{
	std::vector<StackAllocInsn*> stackAllocs;
	IR::FindAllInstructionsOfType(stackAllocs, fn->GetHeadBlock(), HLIR::StackAlloc);

	std::vector<StackAllocInsn*> promotableStackAllocations;

	for (StackAllocInsn* insn : stackAllocs) {
		Value* outputPtr = insn->GetOutputPtr();

		if (outputPtr->GetCountUses() == 0) {
			promotableStackAllocations.push_back(insn);
			continue;
		}

		const Type* allocatedType = insn->GetAllocatedType();

		if (!allocatedType->IsIntegral() && !allocatedType->IsPointer())
			continue;

		bool toRemove = true;

		for (const Use& use : outputPtr->uses()) {
			if (use.GetInstruction() == insn)
				continue;

			if (use.GetInstruction()->GetOpcode() != HLIR::Load
				&& use.GetInstruction()->GetOpcode() != HLIR::Store) {
				toRemove = false;
			}
		}

		if (toRemove) {
			promotableStackAllocations.push_back(insn);
		}
	}

	for (StackAllocInsn* stackAlloc : promotableStackAllocations) {
		Value* outputPtr = stackAlloc->GetOutputPtr();

		VirtualRegisterName* replacementRegister = VirtualRegisterName::Create(stackAlloc->GetAllocatedType());

		std::vector<Instruction*> killList;

		for (const Use& use : outputPtr->uses()) {
			Instruction* user = use.GetInstruction();

			switch (user->GetOpcode()) {
			case HLIR::Load: {
				LoadInsn* load = (LoadInsn*)user;
				killList.push_back(load);

				IR::ReplaceAllUsesWith(load->GetDst(), replacementRegister);
				break;
			}

			case HLIR::Store: {
				StoreInsn* store = (StoreInsn*) user;
				killList.push_back(store);

				SetInsn* set = Helix::CreateSetInsn(replacementRegister, store->GetSrc());
				IR::ReplaceInstructionAndPreserveOriginal(store, set);

				break;
			}
			default:
				break;
			}
		}

		for (Instruction* insn : killList) {
			insn->DeleteFromParent();
		}

		stackAlloc->DeleteFromParent();
	}
}

/*********************************************************************************************************************/

