/* Internal Project Includes */
#include "regalloc2.h"
#include "function.h"
#include "print.h"
#include "interval.h"
#include "arm-md.h" /* generated */
#include "mir.h"
#include "linear-scan.h"
#include "ir-helpers.h"

/* C++ Standard Library Includes */
#include <algorithm>

#pragma optimize("", off)

using namespace Helix;

#define REGALLOC2_DEBUG_LOGS 0

/*********************************************************************************************************************/

static void PrintIntervalTestInfo(Function* function, const std::unordered_map<VirtualRegisterName*, Interval>& intervals)
{
	SlotTracker slots;
	slots.CacheFunction(function);

	// First print the function that we're testing
	Helix::DebugDump(*function);

	// Take all the intervals and sort by their slot indices (basically
	// order of appearance).
	//
	// This is because we use this information in tests & want the data to be "stable".
	// (unordered_maps are not that...)

	std::vector<std::pair<VirtualRegisterName*, Interval>> sorted;
	for (const auto& pair : intervals)
		sorted.push_back(pair);

	std::sort(
		sorted.begin(), sorted.end(),
		[&slots](const std::pair<VirtualRegisterName*, Interval>& lhs, const std::pair<VirtualRegisterName*, Interval>& rhs) {
			return slots.GetValueSlot(lhs.first) < slots.GetValueSlot(rhs.first);
		}
	);

	// Finally print the information to stdout

	fmt::print("********** Interval Analysis **********\n");

	for (const auto [vreg, interval] : sorted) {
		fmt::print("\t%{} = {}:{} -> {}:{}\n",
			slots.GetValueSlot(vreg),
			interval.start.block_index, interval.start.instruction_index,
			interval.end.block_index, interval.end.instruction_index);
	}

	fmt::print("***************************************\n");
}

/*********************************************************************************************************************/

void RegisterAllocator2::Execute(Function* function, const PassRunInformation& info)
{
	//////////////////////////////////////////////////////////////////////////
	// Liveness Analysis
	//////////////////////////////////////////////////////////////////////////

	function->RunLivenessAnalysis();

	//////////////////////////////////////////////////////////////////////////
	// Compute Live Intervals
	//////////////////////////////////////////////////////////////////////////

	std::unordered_map<VirtualRegisterName*, Interval> intervals;
	Helix::ComputeIntervalsForFunction(function, intervals);

	if (info.TestTrace)
		PrintIntervalTestInfo(function, intervals);

	//////////////////////////////////////////////////////////////////////////
	// Reserve any stack space that the IR requests (i.e. manual stack_allocs
	// not register allocator spills)
	// 
	// Do this before the register allocator reserves any stack space for
	// any spills
	//////////////////////////////////////////////////////////////////////////

	StackFrame stackFrame;
	std::unordered_map<StackAllocInsn*, StackFrame::SlotIndex> stackAllocInstructions;

	for (Instruction& insn : *function->GetHeadBlock()) {
		if (insn.GetOpcode() == HLIR::StackAlloc) {
			StackAllocInsn* stackAlloc     = static_cast<StackAllocInsn*>(&insn);
			const size_t    allocationSize = ARMv7::TypeSize(stackAlloc->GetAllocatedType());

			stackAllocInstructions[stackAlloc] = stackFrame.Add(allocationSize);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Allocate each interval a register (or spill)
	//////////////////////////////////////////////////////////////////////////

	LSRA::Context registerAllocatorContext(intervals, &stackFrame);
	LSRA::Run(&registerAllocatorContext);

	// Get the stack size, but aligned to a double word boundary (AAPCS says this
	// is only required at public interfaces, but it should work anywhere anyway)
	// 
	// AAPCS32 (https://github.com/ARM-software/abi-aa/blob/main/aapcs32/aapcs32.rst)
	//
	//  > 6.2.1.2   Stack constraints at a public interface
	//  > ...
	//  >     SP mod 8 = 0. The stack must be double-word aligned.
	//
	const size_t stackSize = stackFrame.GetSizeAligned(8);


	//////////////////////////////////////////////////////////////////////////
	// Rewrite virtual register references to the allocated physical register
	// (or inject spill code)
	//////////////////////////////////////////////////////////////////////////

	// The one, the only, the STACK POINTER ladies and gentlemen
	PhysicalRegisterName* sp = PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::SP);

#if 0
	SlotTracker slots;
	slots.CacheFunction(function);

	for (const auto& [vreg, allocation] : registerAllocatorContext.Allocations) {
		auto interval = intervals[vreg];

		fmt::print("\t%{} ({}:{} -> {}:{}) = {} {}\n",
			slots.GetValueSlot(vreg),
			interval.start.block_index, interval.start.instruction_index,
			interval.end.block_index, interval.end.instruction_index, stringify_operand(allocation.Register, slots), allocation.StackSlot.index);
	}
#endif

	struct Spill
	{
		Instruction*          Insn = nullptr;
		VirtualRegisterName*  VirtualRegister = nullptr;
		StackFrame::SlotIndex Slot;
	};

	// List of places where we need to inject code to load a value from the stack
	std::vector<Spill> loadSpills;

	// List of places where we need to inject code to store a value back to the stack
	std::vector<Spill> restoreSpills;

	for (BasicBlock& bb : function->blocks()) {
		for (Instruction& insn : bb) {
			for (size_t op = 0; op < insn.GetCountOperands(); ++op) {
				if (VirtualRegisterName* vreg = value_cast<VirtualRegisterName>(insn.GetOperand(op))) {
					const auto it = registerAllocatorContext.Allocations.find(vreg);
					
					helix_assert(it != registerAllocatorContext.Allocations.end(),
						"Virtual register has not been allocated a physical register");

					const LSRA::Allocation& allocation = it->second;

					const StackFrame::SlotIndex stackSlot = allocation.StackSlot;

					if (stackSlot.IsValid()) {
						helix_assert(stackFrame.GetAllocationOffset(stackSlot) <= 4095, "stack frame is too large!");

						if (insn.OperandHasFlags(op, Instruction::OP_READ)) {
							loadSpills.push_back({ &insn, vreg, stackSlot });
						}
						else if (insn.OperandHasFlags(op, Instruction::OP_WRITE)) {
							restoreSpills.push_back({ &insn, vreg, stackSlot });
						}
						else {
							helix_unreachable("Value wants to spill but operand is not READ/WRITE marked");
						}

						insn.SetOperand(op, PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::R0));
					}
					else {
						PhysicalRegisterName* physicalRegisterName = allocation.Register;

						helix_assert(PhysicalRegisters::IsValidPhysicalRegister(physicalRegisterName),
							"Value has not been assigned a valid physical register");

						insn.SetOperand(op, physicalRegisterName);
					}
				}
			}
		}
	}

	// Now go and inject any load/store spill code that we need to

	PhysicalRegisterName* r0 = PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::R0);

	for (const Spill& spill : loadSpills) {
		const size_t offset = stackSize - stackFrame.GetAllocationOffset(spill.Slot);
		ConstantInt* offsetValue = ConstantInt::Create(BuiltinTypes::GetInt32(), offset);

		MachineInstruction* ldr = ARMv7::CreateLdri(r0, sp, offsetValue);
		IR::InsertBefore(spill.Insn, ldr);
	}

	for (const Spill& spill : restoreSpills) {
		const size_t offset = stackSize - stackFrame.GetAllocationOffset(spill.Slot);
		ConstantInt* offsetValue = ConstantInt::Create(BuiltinTypes::GetInt32(), offset);

		MachineInstruction* ldr = ARMv7::CreateStri(r0, sp, offsetValue);
		IR::InsertAfter(spill.Insn, ldr);
	}

	// Replace any stack_alloc instructions with LLIR instructions that calculate the correct
	// address from the stack pointer.

	for (const auto& [insn, slot] : stackAllocInstructions) {
		Value*       physicalRegister = insn->GetOutputPtr();
		const size_t offset           = stackSize - stackFrame.GetAllocationOffset(slot);
		ConstantInt* offsetValue      = ConstantInt::Create(BuiltinTypes::GetInt32(), offset);

		Helix::MachineInstruction* addInstruction = ARMv7::CreateAdd_r32i32(sp, offsetValue, physicalRegister);
		
		IR::ReplaceInstructionAndDestroyOriginal(insn, addInstruction);
	}

	// Finally inject function prologue/epilogue code that creates & destroys the stack

	ConstantInt* stack_size_constant = ConstantInt::Create(BuiltinTypes::GetInt32(), stackSize);

	BasicBlock* tail = function->GetTailBlock();
	BasicBlock* head = function->GetHeadBlock();

	// First thing we want to do is 'create' the stack - e.g. move the stack pointer so that it
	// points to the 'bottom' of what we want the stack to be.
	//
	// The emit pass will inject any other prologue code before this (e.g. pushing the frame pointer/link register
	// to the stack etc...)
	head->InsertBefore(head->begin(),                ARMv7::CreateSub_r32i32(sp, stack_size_constant, sp));

	// Finally the last thing to do (before what is presumably the return instruction) is destroy the
	// space on the stack we reserved, meaning that whichever function that comes next can reuse it.
	tail->InsertBefore(tail->Where(tail->GetLast()), ARMv7::CreateAdd_r32i32(sp, stack_size_constant, sp));
}

/*********************************************************************************************************************/
