/**
 * @file match.cpp
 * @author Barney Wilks
 */

#include "match.h"
#include "module.h"
#include "mir.h"
#include "print.h"

using namespace Helix;

/*********************************************************************************************************************/

void MachineExpander::Execute(Module* mod, const PassRunInformation&)
{
	for (Function* fn : mod->functions()) {
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

				bb.Replace(old, insn);
				Helix::DestroyInstruction(old);
			}
		}
	}
}

/*********************************************************************************************************************/
