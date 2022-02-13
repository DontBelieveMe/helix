/* Internal Project Includes */
#include "regalloc2.h"
#include "function.h"
#include "print.h"
#include "interval.h"
#include "arm-md.h" /* generated */
#include "mir.h"
#include "linear-scan.h"

/* C++ Standard Library Includes */
#include <algorithm>

// #pragma optimize("", off)

using namespace Helix;

#define REGALLOC2_DEBUG_LOGS 0

/*********************************************************************************************************************/

struct StackFrame
{
	size_t size = 0;
};

/*********************************************************************************************************************/

static unsigned Align(unsigned input, unsigned alignment)
{
	if (input % alignment == 0) {
		return input;
	}

	return input + (alignment - (input % alignment));
}

/*********************************************************************************************************************/

static size_t AllocateStackSpace(StackFrame* stackFrame, size_t size)
{
	stackFrame->size += size;
	size_t offset = stackFrame->size;
	return offset;
}

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
	// (1) Liveness Analysis
	//////////////////////////////////////////////////////////////////////////

	function->RunLivenessAnalysis();

	//////////////////////////////////////////////////////////////////////////
	// (2) Compute Live Intervals
	//////////////////////////////////////////////////////////////////////////

	std::unordered_map<VirtualRegisterName*, Interval> intervals;
	Helix::ComputeIntervalsForFunction(function, intervals);

	if (info.TestTrace)
		PrintIntervalTestInfo(function, intervals);

	StackFrame stackFrame;
	std::unordered_map<StackAllocInsn*, size_t> variableStackSlots;

	for (Instruction& insn : *function->GetHeadBlock()) {
		if (insn.GetOpcode() == HLIR::StackAlloc) {
			StackAllocInsn* stackAlloc = static_cast<StackAllocInsn*>(&insn);
			variableStackSlots[stackAlloc] = AllocateStackSpace(&stackFrame, ARMv7::TypeSize(stackAlloc->GetAllocatedType()));
		}
	}

	stackFrame.size = Align(stackFrame.size, 8);

	//////////////////////////////////////////////////////////////////////////
	// (3) Allocate each interval a register (or spill)
	//////////////////////////////////////////////////////////////////////////

	LSRA::Context registerAllocatorContext { intervals };
	LSRA::Run(&registerAllocatorContext);

	//////////////////////////////////////////////////////////////////////////
	// (4) Rewrite virtual register references to the allocated physical register
	//     (or inject spill code)
	//////////////////////////////////////////////////////////////////////////

	for (BasicBlock& bb : function->blocks()) {
		for (Instruction& insn : bb) {
			for (size_t op = 0; op < insn.GetCountOperands(); ++op) {
				if (VirtualRegisterName* vreg = value_cast<VirtualRegisterName>(insn.GetOperand(op))) {
					auto it = registerAllocatorContext.Allocated.find(vreg);
					helix_assert(it != registerAllocatorContext.Allocated.end(), "register not allocated for spill :(");
					insn.SetOperand(op, it->second);
				}
			}
		}
	}

	PhysicalRegisterName* sp = PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::SP);

	for (const auto& [insn, slot] : variableStackSlots) {
		Value* preg = insn->GetOutputPtr();

		size_t offset = stackFrame.size - slot;
		ConstantInt* cint = ConstantInt::Create(BuiltinTypes::GetInt32(), offset);
		Helix::MachineInstruction* d = ARMv7::CreateAdd_r32i32(sp, cint, preg);
		BasicBlock* parent = insn->GetParent();

		parent->Replace(insn, d);
	}

	ConstantInt* stack_size_constant = ConstantInt::Create(BuiltinTypes::GetInt32(), stackFrame.size);

	BasicBlock* tail = function->GetTailBlock();
	BasicBlock* head = function->GetHeadBlock();

	head->InsertBefore(head->begin(), ARMv7::CreateSub_r32i32(sp, stack_size_constant, sp));
	tail->InsertBefore(tail->Where(tail->GetLast()), ARMv7::CreateAdd_r32i32(sp, stack_size_constant, sp));
}

/*********************************************************************************************************************/
