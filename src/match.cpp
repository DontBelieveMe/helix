/**
 * @file match.cpp
 * @author Barney Wilks
 */

#include "match.h"
#include "options.h"
#include "module.h"
#include "print.h"
#include "helix.h"
#include "mir.h"

using namespace Helix;

/*********************************************************************************************************************/

void MachineExpander::Execute(Module* mod)
{
	VirtualRegisterName* t0 = VirtualRegisterName::Create(BuiltinTypes::GetInt32());
	VirtualRegisterName* t1 = VirtualRegisterName::Create(BuiltinTypes::GetInt32());
	VirtualRegisterName* t2 = VirtualRegisterName::Create(BuiltinTypes::GetInt32());

	for (Function* fn : mod->functions()) {
		for (BasicBlock& bb : fn->blocks()) {

			BasicBlock::iterator it = bb.begin();
			while (it != bb.end()) {
				Instruction*        old  = &(*it);
				MachineInstruction* insn = ARMv7::Expand(old);// ARMv7::CreateAdd_r32r32(t0, t1, t2);

				it = bb.Where((Instruction*) it->get_next());

				bb.Replace(old, insn);
				Helix::DestroyInstruction(old);
			}
		}
	}
}

/*********************************************************************************************************************/
