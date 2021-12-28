#include "regalloc.h"
#include "basic-block.h"
#include "target-info-armv7.h"
#include "print.h"

#include <algorithm>
#include <utility>

// #pragma optimize("", off)

using namespace Helix;

/*********************************************************************************************************************/

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

/*********************************************************************************************************************/

static unsigned Align(unsigned input, unsigned alignment)
{
	if (input % alignment == 0) {
		return input;
	}

	return input + (alignment - (input % alignment));
}

/*********************************************************************************************************************/

void RegisterAllocator::ComputeStackFrame(StackFrame& frame, Function* fn)
{
	frame.size = 0;

	// At this point all stack allocations should be in the first block
	// so we don't need to scan any other blocks in the function.

	BasicBlock* head = fn->GetHeadBlock();

	for (Instruction& insn : head->insns()) {
		if (insn.GetOpcode() == kInsn_StackAlloc) {
			StackAllocInsn& stack_alloc = (StackAllocInsn&) insn;

			const Type*  allocated_type = stack_alloc.GetAllocatedType();
			const size_t allocated_size = ARMv7::TypeSize(allocated_type);

			frame.size += (unsigned) allocated_size;

			const StackVariable stack_variable {
				(unsigned) allocated_size,
				frame.size,
				&stack_alloc
			};

			frame.variables.push_back(stack_variable);
		}
	}

	frame.size = Align(frame.size, 8);
}

/*********************************************************************************************************************/

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

/*********************************************************************************************************************/

/*
void LowerStackAllocations::Execute(Function* fn)
{
	StackFrame stack_frame;
	this->ComputeStackFrame(stack_frame, fn);

	BasicBlock* head = fn->GetHeadBlock();
	BasicBlock* tail = fn->GetTailBlock();

	helix_assert(head && tail, "requires head & tail blocks");

	PhysicalRegisterName* sp = PhysicalRegisters::GetRegister(PhysicalRegisters::SP);

	for (const StackVariable& stack_var : stack_frame.variables) {
		const unsigned offset = stack_frame.size - stack_var.offset;

		BasicBlock::iterator where = head->Where(stack_var.alloca_insn);

		ConstantInt*         offset_value = ConstantInt::Create(BuiltinTypes::GetInt32(), offset);
		VirtualRegisterName* temp         = VirtualRegisterName::Create(BuiltinTypes::GetInt32());
		Value*               output_ptr   = stack_var.alloca_insn->GetOutputPtr();

		where = head->InsertAfter(where, Helix::CreateBinOp(kInsn_IAdd, sp, offset_value, temp));
		where = head->InsertAfter(where, Helix::CreateIntToPtr(BuiltinTypes::GetInt32(), temp, output_ptr));

		head->Remove(head->Where(stack_var.alloca_insn));
	}

	ConstantInt* stack_size_constant = ConstantInt::Create(BuiltinTypes::GetInt32(), stack_frame.size);

	head->InsertBefore(head->begin(), Helix::CreateBinOp(kInsn_ISub, sp, stack_size_constant, sp));
	tail->InsertBefore(tail->begin(), Helix::CreateBinOp(kInsn_IAdd, sp, stack_size_constant, sp));
}
*/

/*********************************************************************************************************************/
