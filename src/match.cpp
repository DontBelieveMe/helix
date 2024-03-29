/**
 * @file match.cpp
 * @author Barney Wilks
 */

/* Internal Project Includes */
#include "match.h"
#include "module.h"
#include "mir.h"
#include "ir-helpers.h"

using namespace Helix;

/******************************************************************************/

void MachineExpander::Execute(Function* fn, const PassRunInformation&)
{
	for (BasicBlock& bb : fn->blocks()) {
		BasicBlock::iterator it = bb.begin();

		while (it != bb.end()) {
			Instruction* old = &(*it);

			if (old->GetOpcode() == HLIR::StackAlloc
				|| Helix::IsMachineOpcode(old->GetOpcode())) {
				it = bb.Where((Instruction*) old->get_next());
				continue;
			}

			MachineInstruction* insn = ARMv7::Expand(old);

			it = bb.Where((Instruction*) it->get_next());

			if (insn->GetParent() != old->GetParent())
				bb.Replace(old, insn);

			IR::DestroyInstruction(old);
		}
	}
}

/******************************************************************************/
