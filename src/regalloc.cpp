#include "regalloc.h"
#include "basic-block.h"
#include "target-info-armv7.h"
#include "print.h"

#include <algorithm>
#include <utility>

// #pragma optimize("", off)

using namespace Helix;

void RegisterAllocator::Execute(BasicBlock* bb)
{
	if (!Options::GetEnableExperimentalRegisterAllocator())
		return;

	struct Reg { VirtualRegisterName* reg; size_t freq; PhysicalRegisters::ArmV7RegisterID physical_id; bool spill=true; };
	std::unordered_map<VirtualRegisterName*, size_t> frequencyMap;

	SlotTracker slotTracker;

	for (Instruction& insn : bb->insns()) {
		for (size_t opIndex = 0; opIndex < insn.GetCountOperands(); ++opIndex) {
			Value* op = insn.GetOperand(opIndex);
			slotTracker.GetValueSlot(op);

			if (value_isa<VirtualRegisterName>(op)) {
				frequencyMap[(VirtualRegisterName*) op] = op->GetCountUses();
			}
		}
	}

	std::vector<Reg> freqs;
	for (const auto& pair : frequencyMap) {
		freqs.push_back({pair.first, pair.second});
	}

	PhysicalRegisters::ArmV7RegisterID allowed_registers[] =
	{
		PhysicalRegisters::R4,
		PhysicalRegisters::R5,
		PhysicalRegisters::R6,
		PhysicalRegisters::R7,
		PhysicalRegisters::R8,
		PhysicalRegisters::R10
	};

	std::sort(freqs.begin(), freqs.end(), [](const Reg& a, const Reg& b) { return a.freq > b.freq; });

	std::unordered_map<VirtualRegisterName*, Reg> allocated_regs;

	for (size_t i = 0; i < std::min(6ull, freqs.size()); ++i) {
		freqs[i].spill = false;
		freqs[i].physical_id = allowed_registers[i];
		allocated_regs[freqs[i].reg] = freqs[i];

		// size_t slot = slotTracker.GetValueSlot(freqs[i].reg);
		// printf("allocating %%%zu -> %s (freq=%zu)\n", slot, PhysicalRegisters::GetRegisterString(freqs[i].physical_id), freqs[i].freq);
	}

	for (Instruction& insn : bb->insns()) {
		for (size_t opIndex = 0; opIndex < insn.GetCountOperands(); ++opIndex) {
			Value* op = insn.GetOperand(opIndex);

			if (value_isa<VirtualRegisterName>(op)) {
				const Reg& allocation = allocated_regs[(VirtualRegisterName*) op];

				helix_assert(!allocation.spill, "Spilling of register -> memory not supported");

				if (!allocation.spill) {
					insn.SetOperand(opIndex, PhysicalRegisters::GetRegister(allocation.physical_id));
				}
			}
		}
	}
}
