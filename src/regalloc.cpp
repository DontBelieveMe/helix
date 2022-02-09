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

	if (type->IsStruct()) {
		const size_t structSize = ARMv7::TypeSize(type);
		return structSize <= 4;
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
		if (insn.GetOpcode() == HLIR::StackAlloc) {
			StackAllocInsn& stack_alloc = (StackAllocInsn&) insn;

			const Type*  allocated_type = stack_alloc.GetAllocatedType();
			const size_t allocated_size = ARMv7::TypeSize(allocated_type);

			frame.size += (unsigned) allocated_size;

			const StackVariable stack_variable {
				(unsigned) allocated_size,
				frame.size,
				&stack_alloc
			};

			helix_debug(logs::regalloc, "@ {}={} bytes, +{}", GetTypeName(allocated_type), allocated_size, stack_variable.offset);

			frame.variables.push_back(stack_variable);
		}
	}

	frame.size = Align(frame.size, 8);
}

/*********************************************************************************************************************/

static const Type* LowerVirtualRegisterType(const Type* type) {
	if (type->IsPointer()) {
		return BuiltinTypes::GetInt32();
	}
	else if (type->IsIntegral()) {
		return type;
	} else if (type->IsStruct()) {
		if (ARMv7::TypeSize(type) <= 4) {
			return BuiltinTypes::GetInt32();
		}
	}

	helix_unreachable("type cannot be lowered into a single physical register");

	return nullptr;
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
		PhysicalRegisters::R6
	};

	size_t nextAvailablePhysicalRegister = 0;
	
	struct Spill
	{
		size_t spillIndex;
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
			if (use.GetInstruction()->GetOpcode() == HLIR::StackAlloc) {
				return true;
			}
		}

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

	size_t nextSpillID = 0;

	for (BasicBlock& bb : fn->blocks()) {
		for (Instruction& insn : bb.insns()) {
			if (insn.GetOpcode() == HLIR::StackAlloc) {
				continue;
			}

			std::vector<size_t> operands;

			for (size_t operandIndex = 0; operandIndex < insn.GetCountOperands(); ++operandIndex) {
				operands.push_back(operandIndex);
			}

			for (size_t operandIndex : operands) {
				Value* operand = insn.GetOperand(operandIndex);

				if (VirtualRegisterName* vreg = value_cast<VirtualRegisterName>(operand)) {
					helix_assert(CanTypeFitInNativeRegister(vreg->GetType()), "type is too big for native regiser (e.g. 32 bits)");

					const Type* preg_type = LowerVirtualRegisterType(vreg->GetType());

					PhysicalRegisterName* physical_register
						= PhysicalRegisters::GetRegister(preg_type, kPhysicalRegisterIDs[nextAvailablePhysicalRegister]);

					nextAvailablePhysicalRegister++;

					if (IsStackAllocation(vreg)) {
						continue;
					}

					auto it = spills.find(vreg);

					const Spill* spill_ref = nullptr;

					if (it == spills.end()) {
						const Spill spill { nextSpillID, vreg->GetType(), VirtualRegisterName::Create(BuiltinTypes::GetPointer()) };
						spills.insert({vreg, spill});
						spill_ref = &spills[vreg];
						nextSpillID++;
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
		auto i = Helix::CreateLoad(load.spill_ref->mem_addr, load.physical_reg);
		i->SetComment("restore");
		load.bb.InsertBefore(where, i);
	}

	for (SpillInstance& store : stores) {
		BasicBlock::iterator where = store.bb.Where(&store.insn);
		auto i = Helix::CreateStore(store.physical_reg, store.spill_ref->mem_addr);
		i->SetComment("spill");
		store.bb.InsertAfter(where, i);
	}

	BasicBlock* head = fn->GetHeadBlock();

	std::vector<Spill> sortedSpills;
	sortedSpills.reserve(spills.size());
	
	for (auto it = spills.begin(); it != spills.end(); ++it) {
		sortedSpills.push_back(it->second);
	}

	std::sort(sortedSpills.begin(), sortedSpills.end(), [](const Spill& a, const Spill& b) { return a.spillIndex < b.spillIndex; });

	for (const Spill& spill : sortedSpills) {
		auto i = Helix::CreateStackAlloc(spill.mem_addr, spill.ty);
		i->SetComment("spilled temp");
		head->InsertBefore(head->begin(), i);
	}

	StackFrame stack_frame;
	this->ComputeStackFrame(stack_frame, fn);

	BasicBlock* tail = fn->GetTailBlock();

	PhysicalRegisterName* sp = PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::SP);

	// Used when the stack offest > 255 (& cannot fit in an add immediate operand)
	std::unordered_map<unsigned, GlobalVariable*> globalStackOffsets;

	PhysicalRegisterName* r7 = PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::R7);

	for (const StackVariable& stack_var : stack_frame.variables) {
		const unsigned offset = stack_frame.size - stack_var.offset;

		GlobalVariable* global = nullptr;
		ConstantInt* offset_value_constant = nullptr;

		if (offset > 255) {
			auto globalIterator = globalStackOffsets.find(offset);

			if (globalIterator == std::end(globalStackOffsets)) {
				static size_t index = 0;
				const std::string name = "so" + std::to_string(index);
				index++;

				offset_value_constant = ConstantInt::Create(BuiltinTypes::GetInt32(), offset);

				global = GlobalVariable::Create(name, BuiltinTypes::GetInt32(), offset_value_constant);
				fn->GetParent()->RegisterGlobalVariable(global);

				globalStackOffsets[offset] = global;

			} else {
				global = globalIterator->second;
			}
		} else {
			offset_value_constant = ConstantInt::Create(BuiltinTypes::GetInt32(), offset);
		}

		helix_debug(logs::regalloc, "Result Offset: {} (Stack Frame: {}, Var Offset: {})", offset, stack_frame.size, stack_var.offset);

		struct T { Use use; PhysicalRegisterName* reg; };

		Value* output_ptr = stack_var.alloca_insn->GetOutputPtr();
		int c = 0;
		std::vector<T> worklist;

		for (Use& use : output_ptr->uses()) {
			if (use.GetInstruction() == stack_var.alloca_insn)
				continue;

			BasicBlock::iterator where = head->Where(use.GetInstruction());

			Value* offsetValue = nullptr;

			if (global) {
				head->InsertBefore(where, Helix::CreateLoad(global, r7));
				offsetValue = r7;
			} else {
				offsetValue = offset_value_constant;
			}

			PhysicalRegisterName* output = PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), kAvailableAddressRegisters[c % 2]);
			head->InsertBefore(where, Helix::CreateBinOp(HLIR::IAdd, sp, offsetValue, output));
			c++;
			worklist.push_back({use, output});
		}


		for (T& use : worklist) {
			use.use.ReplaceWith(use.reg);
		}

		head->Remove(head->Where(stack_var.alloca_insn));
	}

	ConstantInt* stack_size_constant = ConstantInt::Create(BuiltinTypes::GetInt32(), stack_frame.size);

	head->InsertBefore(head->begin(), Helix::CreateBinOp(HLIR::ISub, sp, stack_size_constant, sp));

	tail->InsertBefore(tail->Where(tail->GetLast()), Helix::CreateBinOp(HLIR::IAdd, sp, stack_size_constant, sp));
}

/*********************************************************************************************************************/
