#include "regalloc.h"
#include "basic-block.h"
#include "target-info-armv7.h"

using namespace Helix;

void RegisterAllocator::Execute(BasicBlock* bb)
{
	for (Instruction& insn : bb->insns()) {
		for (size_t opIndex = 0; opIndex < insn.GetCountOperands(); ++opIndex) {
			Value* op = insn.GetOperand(opIndex);

			if (value_isa<VirtualRegisterName>(op)) {
				//PhysicalRegisterName* r4 = PhysicalRegisters::GetRegister(PhysicalRegisters::R4);
				//insn.SetOperand(opIndex, r4);
			}
		}
	}
}
