/* Internal Project Includes */
#include "regalloc2.h"
#include "function.h"
#include "print.h"
#include "ir-helpers.h"
#include "instruction-index.h"
#include "interval.h"

/* C Standard Library Includes */
#include <ctype.h>

/* C++ Standard Library Includes */
#include <algorithm>

#pragma optimize("", off)

using namespace Helix;

#define REGALLOC2_DEBUG_LOGS 0

/*********************************************************************************************************************/

static size_t NewStackLocation(size_t* stackCounter)
{
	const size_t tmp = *stackCounter;
	*stackCounter = *stackCounter + 1;
	return tmp;
}

/*********************************************************************************************************************/

static void SpillAtInterval(size_t* stackCounter, std::vector<Interval>* active, Interval* interval)
{
	Interval& spill = active->back();

	if (!IntervalEndComparator()(spill, *interval)) {
		interval->physical_register = spill.physical_register;
		spill.stack_slot = NewStackLocation(stackCounter);

		active->erase(std::remove(active->begin(), active->end(), spill), active->end());
		active->push_back(*interval);

		std::sort(active->begin(), active->end(), IntervalEndComparator());
	}
	else {
		interval->stack_slot = NewStackLocation(stackCounter);
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

void RegisterAllocator2::Execute(Function* function)
{
	//////////////////////////////////////////////////////////////////////////
	// (1) Liveness Analysis
	//////////////////////////////////////////////////////////////////////////

	function->RunLivenessAnalysis();

#if REGALLOC2_DEBUG_LOGS
	SlotTracker slots;
	slots.CacheFunction(function);

	helix_debug(logs::regalloc2, "********** Liveness Analysis **********");

	for (const BasicBlock& bb : function->blocks()) {
		helix_debug(logs::regalloc2, "bb{}", slots.GetBasicBlockSlot(&bb));

		const auto in = bb.GetLiveIn();
		const auto out = bb.GetLiveOut();

		helix_debug(logs::regalloc2, "\tIN [count={}]:", in.size());
		for (const VirtualRegisterName* v : in) {
			helix_debug(logs::regalloc2, "\t   - %{}", slots.GetValueSlot(v));
		}

		helix_debug(logs::regalloc2, "\tOUT [count={}]:", out.size());
		for (const VirtualRegisterName* v : out) {
			helix_debug(logs::regalloc2, "\t   - %{}", slots.GetValueSlot(v));
		}
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// (2) Compute Live Intervals
	//////////////////////////////////////////////////////////////////////////

	std::unordered_map<VirtualRegisterName*, Interval> intervals;
	Helix::ComputeIntervalsForFunction(function, intervals);

#if REGALLOC2_DEBUG_LOGS
	helix_debug(logs::regalloc2, "********** Interval Analysis **********");

	for (const auto [vreg, interval] : intervals) {
		helix_debug(logs::regalloc2, "%{} = {}:{} -> {}:{}",
			slots.GetValueSlot(vreg),
			interval.start.block_index, interval.start.instruction_index,
			interval.end.block_index, interval.end.instruction_index);
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// (3) Allocate each interval a register (or spill)
	//////////////////////////////////////////////////////////////////////////

	std::vector<Interval> intervals_sorted;
	
	{

		for (const auto& [vreg, interval] : intervals) {
			intervals_sorted.push_back(interval);
		}

		std::sort(intervals_sorted.begin(), intervals_sorted.end(), IntervalStartComparator());

		std::vector<Interval> active;
		size_t stackCounter = 0;

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
				SpillAtInterval(&stackCounter, &active, &interval);
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
}

/*********************************************************************************************************************/
