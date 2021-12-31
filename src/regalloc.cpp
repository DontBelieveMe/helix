#include "regalloc.h"
#include "basic-block.h"
#include "target-info-armv7.h"
#include "print.h"

#include <algorithm>
#include <utility>

#pragma optimize("", off)

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
	PhysicalRegisters::ArmV7RegisterID kPhysicalRegisterIDs[] =
	{
		PhysicalRegisters::R0,
		PhysicalRegisters::R1,
		PhysicalRegisters::R2,
		PhysicalRegisters::R3,
		PhysicalRegisters::R4,
		PhysicalRegisters::R5,
		PhysicalRegisters::R6,
		PhysicalRegisters::R7,
	};

	size_t nextAvailablePhysicalRegister = 0;
	
	struct Spill
	{
		const Type* ty;
		VirtualRegisterName* mem_addr;
	};

	std::unordered_map<VirtualRegisterName*, const Spill> spills;

	struct SpillInstance
	{
		BasicBlock& bb;
		Instruction& insn;
		size_t       operandIndex;
		const Spill* spill_ref;
		PhysicalRegisterName* physical_reg;
	};

	auto IsStackAllocation = [fn, &spills](Value* v) -> bool {
		for (Use& use : v->uses()) {
			if (use.GetInstruction()->GetOpcode() == kInsn_StackAlloc) {
				return true;
			}
		}

		//if (VirtualRegisterName* vreg = value_cast<VirtualRegisterName>(v)) {
		//	auto it = spills.find(vreg);
//
		//	if (it != spills.end())
		//		return true;
		//}

		return false;
	};

	std::vector<SpillInstance> loads;
	std::vector<SpillInstance> stores;

	PhysicalRegisters::ArmV7RegisterID kAvailableAddressRegisters[] =
	{
		PhysicalRegisters::R10,
		PhysicalRegisters::R8,
	};

	std::unordered_map<Value*, PhysicalRegisterName*> addressRegisters;

	size_t nextAvailableAddressRegister = 0;

	for (BasicBlock& bb : fn->blocks()) {
		for (Instruction& insn : bb.insns()) {
			if (insn.GetOpcode() == kInsn_StackAlloc/* || insn.GetOpcode() == kInsn_Load ||
			    insn.GetOpcode() == kInsn_Store*/) {
				continue;
			}

			std::vector<size_t> operands;

			//if (insn.GetOpcode() == kInsn_Load) {
			//	operands.push_back(1);
			//} else if (insn.GetOpcode() == kInsn_Store) {
			//	operands.push_back(0);
			//} else
			{
				for (size_t operandIndex = 0; operandIndex < insn.GetCountOperands(); ++operandIndex) {
					operands.push_back(operandIndex);
				}
			}

			for (size_t operandIndex : operands) {
				Value* operand = insn.GetOperand(operandIndex);

				if (VirtualRegisterName* vreg = value_cast<VirtualRegisterName>(operand)) {
					helix_assert(CanTypeFitInNativeRegister(vreg->GetType()), "type is too big for native regiser (e.g. 32 bits)");

					PhysicalRegisterName* physical_register = PhysicalRegisters::GetRegister(kPhysicalRegisterIDs[nextAvailablePhysicalRegister]);
					nextAvailablePhysicalRegister++;

					if (IsStackAllocation(vreg)) {
						//insn.SetOperand(operandIndex, PhysicalRegisters::GetRegister(PhysicalRegisters::R10));
						//addressRegisters[(Value*)vreg] = PhysicalRegisters::GetRegister(kAvailableAddressRegisters[nextAvailableAddressRegister]);
						//nextAvailableAddressRegister++;
						continue;
					}

					auto it = spills.find(vreg);

					const Spill* spill_ref = nullptr;

					if (it == spills.end()) {
						const Spill spill { vreg->GetType(), VirtualRegisterName::Create(BuiltinTypes::GetPointer()) };
						spills.insert({vreg, spill});
						spill_ref = &spills[vreg];
					} else {
						spill_ref = &it->second;
					}

					if (insn.OperandHasFlags(operandIndex, Instruction::OP_READ)) {
						loads.push_back({bb, insn, operandIndex, spill_ref,physical_register});
					} else if (insn.OperandHasFlags(operandIndex, Instruction::OP_WRITE)) {
						stores.push_back({bb, insn, operandIndex, spill_ref,physical_register});
					}

					insn.SetOperand(operandIndex, physical_register);
				}
			}

			nextAvailablePhysicalRegister = 0;
			nextAvailableAddressRegister = 0;
		}
	}

	for (SpillInstance& load : loads) {
		BasicBlock::iterator where = load.bb.Where(&load.insn);
		//PhysicalRegisterName* output = addressRegisters[load.spill_ref->mem_addr];
		auto i = Helix::CreateLoad(load.spill_ref->mem_addr, load.physical_reg);
		//auto i = Helix::CreateLoad(output, load.physical_reg);
		i->SetComment("restore");
		load.bb.InsertBefore(where, i);
	}

	for (SpillInstance& store : stores) {
		BasicBlock::iterator where = store.bb.Where(&store.insn);
		//PhysicalRegisterName* output = addressRegisters[store.spill_ref->mem_addr];
		auto i = Helix::CreateStore(store.physical_reg, store.spill_ref->mem_addr);
		//auto i = Helix::CreateLoad(output, store.physical_reg);
		i->SetComment("spill");
		store.bb.InsertAfter(where, i);
	}

	BasicBlock* head = fn->GetHeadBlock();

	for (const auto& spillPair : spills) {
		const Spill& spill = spillPair.second;
		auto i = Helix::CreateStackAlloc(spill.mem_addr, spill.ty);
		i->SetComment("spilled temp");
		head->InsertBefore(head->begin(), i);
	}

	StackFrame stack_frame;
	this->ComputeStackFrame(stack_frame, fn);

	BasicBlock* tail = fn->GetTailBlock();

	PhysicalRegisterName* sp = PhysicalRegisters::GetRegister(PhysicalRegisters::SP);

	for (const StackVariable& stack_var : stack_frame.variables) {
		const unsigned offset = stack_frame.size - stack_var.offset;
		ConstantInt*         offset_value = ConstantInt::Create(BuiltinTypes::GetInt32(), offset);

		Value*               output_ptr   = stack_var.alloca_insn->GetOutputPtr();

		///*BasicBlock::iterator where = head->Where(stack_var.alloca_insn);
//
		//VirtualRegisterName* temp         = VirtualRegisterName::Create(BuiltinTypes::GetInt32());
//
		//where = head->InsertAfter(where, Helix::CreateBinOp(kInsn_IAdd, sp, offset_value, temp));
		//where = head->InsertAfter(where, Helix::CreateIntToPtr(BuiltinTypes::GetInt32(), temp, output_ptr));
//
//
		Instruction* lastInsn = output_ptr->GetUse(0).GetInstruction();
		int c = 0;

		struct T { Use use; PhysicalRegisterName* reg; };
		std::vector<T> worklist;

		for (Use& use : output_ptr->uses()) {
			if (use.GetInstruction() == stack_var.alloca_insn)
				continue;

			BasicBlock::iterator where = head->Where(use.GetInstruction());

			PhysicalRegisterName* output = PhysicalRegisters::GetRegister(kAvailableAddressRegisters[c % 2]);  //addressRegisters[use.GetInstruction()->GetOperand(use.GetOperandIndex())];
			//PhysicalRegisterName* output = addressRegisters[use.GetInstruction()->GetOperand(use.GetOperandIndex())];
			head->InsertBefore(where, Helix::CreateBinOp(kInsn_IAdd, sp, offset_value, output));
			//use.GetInstruction()->SetOperand(use.GetOperandIndex(), output);
			c++;
			worklist.push_back({use, output});
		}


		for (T& use : worklist) {
			use.use.ReplaceWith(use.reg);
		}

		head->Remove(head->Where(stack_var.alloca_insn));
	}

	ConstantInt* stack_size_constant = ConstantInt::Create(BuiltinTypes::GetInt32(), stack_frame.size);

	head->InsertBefore(head->begin(), Helix::CreateBinOp(kInsn_ISub, sp, stack_size_constant, sp));

	tail->InsertBefore(tail->Where(tail->GetLast()), Helix::CreateBinOp(kInsn_IAdd, sp, stack_size_constant, sp));
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
