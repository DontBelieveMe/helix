/**
 * @file linear-scan.cpp
 * @author Barney Wilks
 *
 * Implements linear scan register allocation (linear-scan.h)
 */

/* Internal Project Includes */
#include "linear-scan.h"
#include "value.h"
#include "target-info-armv7.h"
#include "system.h"

#include <vector>
#include <algorithm>
#include <unordered_set>

using namespace Helix;

#pragma optimize("", off)

/*********************************************************************************************************************/

LSRA::Context::Context(const std::unordered_map<VirtualRegisterName*, Interval>& intervals, StackFrame* stack)
	: InputIntervals(intervals), Stack(stack)
{ }

/*********************************************************************************************************************/

static void PopulateAllAvailableRegisters(std::unordered_set<PhysicalRegisterName*>& registers)
{
	registers = {
		PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::R4),
		PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::R5),
		PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::R6),
		PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::R7),
		PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::R8),
	};
}

/*********************************************************************************************************************/

static void SpillAtInterval(StackFrame* stack, std::vector<Interval*>* active, Interval* interval)
{
	Interval* spill = active->back();

	// If this interval ends before the one with largest (furthest away) end point in the active list
	// then spill that interval instead, and use its register for ourselves.
	//
	// This is a fairly simple heuristic but apparently gives good enough results.
	if (!IntervalEndComparator()(*spill, *interval)) {
		const size_t spillSize = ARMv7::TypeSize(spill->virtual_register->GetType());

		// Assign the register of the interval we're spilling instead...
		interval->physical_register = spill->physical_register;

		// ... and spill that interval instead.
		spill->stack_slot = stack->Add(spillSize);
		spill->physical_register = nullptr;

		// Remove the interval we've just spilled from the active list and push our interval
		// (the that 'stole' the register) to that list.
		active->erase(std::remove(active->begin(), active->end(), spill), active->end());
		active->push_back(interval);

		// Make sure the active list stays sorted by increasing end points.
		std::sort(active->begin(), active->end(), [](Interval* a, Interval* b) { return IntervalEndComparator()(*a, *b); });
	}
	else {
		// Otherwise just spill this interval to the stack anyway

		const size_t spillSize = ARMv7::TypeSize(interval->virtual_register->GetType());

		interval->stack_slot = stack->Add(spillSize);
		interval->physical_register = nullptr;
	}
}

/*********************************************************************************************************************/

static void ExpireOldIntervals(std::unordered_set<PhysicalRegisterName*>* freeRegisters,
	std::vector<Interval*>* active, Interval* interval)
{
	std::vector<Interval*> keep;

	for (Interval* active_interval : *active) {
		if (!IntervalEndStartComparator()(*active_interval, *interval)) {
			keep.push_back(active_interval);
			continue;
		}

		freeRegisters->insert(active_interval->physical_register);
	}

	*active = keep;
	std::sort(active->begin(), active->end(), [](Interval* a, Interval* b) { return IntervalEndComparator()(*a, *b); });
}

/*********************************************************************************************************************/

void LSRA::Run(Context* context)
{
	// (1) Sort the intervals in order of increasing end point

	std::vector<Interval> intervals;
	intervals.reserve(context->InputIntervals.size());

	for (const auto& [vreg, interval] : context->InputIntervals)
		intervals.push_back(interval);

	std::sort(intervals.begin(), intervals.end(), IntervalStartComparator());

	// (2) Define a list of intervals 'active' - a list of intervals that overlap
	//     at the current point in the program & have been placed in registers.
	//     Active should be kept sorted in order of increasing end point
	std::vector<Interval*> active;

	// (3) Define a list of all free registers & define 'R' to be the total
	//     number of registers that could possibly be free at any one time.
	std::unordered_set<PhysicalRegisterName*> freeRegisters;
	PopulateAllAvailableRegisters(freeRegisters);

	const size_t R = freeRegisters.size();

	// (4) For each interval in the program
	for (Interval& interval : intervals) {
		// (5) Remove any “expired” intervals - those intervals that no
		//     longer overlap the new interval because their end point precedes the new intervals
		//     start point (and then make the expired intervals register free)
		ExpireOldIntervals(&freeRegisters, &active, &interval);

		if (active.size() >= R) {
			SpillAtInterval(context->Stack, &active, &interval);
		}
		else {
			PhysicalRegisterName* preg = *(freeRegisters.begin());
			freeRegisters.erase(preg);

			interval.physical_register = preg;
			active.push_back(&interval);
			std::sort(active.begin(), active.end(), [](Interval* a, Interval* b) { return IntervalEndComparator()(*a, *b); });
		}
	}

	for (Interval& interval : intervals) {
		context->Allocations[interval.virtual_register] = { interval.physical_register, interval.stack_slot };
	}
}

/*********************************************************************************************************************/
