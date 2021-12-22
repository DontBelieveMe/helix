#include "regalloc.h"
#include "basic-block.h"
#include "target-info-armv7.h"
#include "print.h"

#include <algorithm>
#include <utility>

// #pragma optimize("", off)

using namespace Helix;

static bool CanTypeFitInNativeRegister(const Type* type)
{
	if (const IntegerType* int_type = type_cast<IntegerType>(type)) {
		return int_type->GetBitWidth() <= 32;
	}

	if (type->IsPointer()) {
		return true;
	}

	return false;
}

void RegisterAllocator::Execute(Function* fn)
{
	PhysicalRegisters::ArmV7RegisterID available_registers[] =
	{
		PhysicalRegisters::R0,
		PhysicalRegisters::R1,
		PhysicalRegisters::R2,
		PhysicalRegisters::R3,
		PhysicalRegisters::R4,
		PhysicalRegisters::R5,
		PhysicalRegisters::R6,
		PhysicalRegisters::R7,
		PhysicalRegisters::R8,
		PhysicalRegisters::R10
	};

	size_t top = 0;

	std::unordered_map<VirtualRegisterName*, PhysicalRegisterName*> register_allocation;

	for (BasicBlock& bb : fn->blocks()) {
		for (Instruction& insn : bb.insns()) {
			for (size_t operandIndex = 0; operandIndex < insn.GetCountOperands(); ++operandIndex) {
				Value* operand = insn.GetOperand(operandIndex);

				if (VirtualRegisterName* vreg = value_cast<VirtualRegisterName>(operand)) {
					auto it = register_allocation.find(vreg);

					PhysicalRegisterName* physical_register = nullptr;

					if (it == register_allocation.end()) {
						helix_assert(top < 10, "cannot allocate register");
						helix_assert(CanTypeFitInNativeRegister(operand->GetType()), "type of virtual register cannot fit in native register");

						physical_register = PhysicalRegisters::GetRegister(available_registers[top]);
						top++;

						register_allocation[vreg] = physical_register;
					} else {
						physical_register = it->second;
					}

					insn.SetOperand(operandIndex, physical_register);
				}
			}
		}
	}
}
