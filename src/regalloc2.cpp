/* Internal Project Includes */
#include "regalloc2.h"
#include "function.h"
#include "print.h"
#include "ir-helpers.h"
#include "instruction-index.h"
#include "interval.h"
#include "arm-md.h" /* generated */
#include "mir.h"

/* C Standard Library Includes */
#include <ctype.h>

/* C++ Standard Library Includes */
#include <algorithm>

#pragma optimize("", off)

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

static void SpillAtInterval(StackFrame* stackFrame, std::vector<Interval>* active, Interval* interval)
{
	helix_unreachable("spilling temporarily unavailable");

	Interval& spill = active->back();

	if (!IntervalEndComparator()(spill, *interval)) {
		interval->physical_register = spill.physical_register;
		spill.stack_slot = AllocateStackSpace(stackFrame, ARMv7::TypeSize(spill.virtual_register->GetType()));

		active->erase(std::remove(active->begin(), active->end(), spill), active->end());
		active->push_back(*interval);

		std::sort(active->begin(), active->end(), IntervalEndComparator());
	}
	else {
		interval->stack_slot = AllocateStackSpace(stackFrame, ARMv7::TypeSize(interval->virtual_register->GetType()));
	}
}

/*********************************************************************************************************************/

static void ExpireOldIntervals(std::set<PhysicalRegisterName*>* free_regs, std::vector<Interval>* active, Interval* interval)
{
	std::vector<Interval> keep;

	for (const Interval& active_interval : *active) {
		if (!IntervalEndStartComparator()(active_interval, *interval)) {
			keep.push_back(active_interval);
			continue;
		}
		
		free_regs->insert(active_interval.physical_register);
	}

	*active = keep;
	std::sort(active->begin(), active->end(), IntervalEndComparator());
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

	//////////////////////////////////////////////////////////////////////////
	// (3) Allocate each interval a register (or spill)
	//////////////////////////////////////////////////////////////////////////

	std::vector<Interval> intervals_sorted;
	StackFrame stackFrame;

	std::unordered_map<StackAllocInsn*, size_t> variableStackSlots;

	for (Instruction& insn : *function->GetHeadBlock()) {
		if (insn.GetOpcode() == HLIR::StackAlloc) {
			StackAllocInsn* stackAlloc = static_cast<StackAllocInsn*>(&insn);
			variableStackSlots[stackAlloc] = AllocateStackSpace(&stackFrame, ARMv7::TypeSize(stackAlloc->GetAllocatedType()));
		}
	}

	{
		for (const auto& [vreg, interval] : intervals) {
			intervals_sorted.push_back(interval);
		}

		std::sort(intervals_sorted.begin(), intervals_sorted.end(), IntervalStartComparator());

		std::vector<Interval> active;

		std::set<PhysicalRegisterName*> free_registers =
		{
			PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::R4),
			PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::R5),
			PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::R6),
			PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::R7),
			PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::R8),
		};

		const size_t kTotalAvailableRegisters = free_registers.size();

		for (Interval& interval : intervals_sorted) {
			ExpireOldIntervals(&free_registers, &active, &interval);

			if (active.size() >= kTotalAvailableRegisters) {
				SpillAtInterval(&stackFrame, &active, &interval);
			}
			else {
				PhysicalRegisterName* preg = *free_registers.begin();
				free_registers.erase(preg);

				interval.physical_register = preg;
				active.push_back(interval);
				std::sort(active.begin(), active.end(), IntervalEndComparator());
			}
		}
	}

#if REGALLOC2_DEBUG_LOGS
	helix_debug(logs::regalloc2, "********** Final Allocation **********");

	for (const Interval& interval : intervals_sorted)
	{
		helix_debug(logs::regalloc2, "%{} = {}:{} -> {}:{} (reg={}, stack={})",
			slots.GetValueSlot(interval.virtual_register),
			interval.start.block_index, interval.start.instruction_index,
			interval.end.block_index, interval.end.instruction_index,
			stringify_operand(interval.physical_register, slots), interval.stack_slot);
	}
#endif

	stackFrame.size = Align(stackFrame.size, 8);

	//////////////////////////////////////////////////////////////////////////
	// (4) Rewrite virtual register references to the allocated physical register
	//     (or inject spill code)
	//////////////////////////////////////////////////////////////////////////

	std::unordered_map<VirtualRegisterName*, PhysicalRegisterName*> map;

	for (const Interval& interval : intervals_sorted)
		map.insert({ interval.virtual_register, interval.physical_register });

	for (BasicBlock& bb : function->blocks()) {
		for (Instruction& insn : bb) {
			for (size_t op = 0; op < insn.GetCountOperands(); ++op) {
				if (VirtualRegisterName* vreg = value_cast<VirtualRegisterName>(insn.GetOperand(op))) {
					auto it = map.find(vreg);
					helix_assert(it != map.end(), "register not allocated for spill :(");
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
