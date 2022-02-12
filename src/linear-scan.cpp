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

static void SpillAtInterval(std::vector<Interval>* active, Interval* interval)
{
	helix_unreachable("spilling temporarily unavailable");

#if 0
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
#endif
}

/*********************************************************************************************************************/

static void ExpireOldIntervals(std::unordered_set<PhysicalRegisterName*>* freeRegisters,
	std::vector<Interval>* active, Interval* interval)
{
	std::vector<Interval> keep;

	for (const Interval& active_interval : *active) {
		if (!IntervalEndStartComparator()(active_interval, *interval)) {
			keep.push_back(active_interval);
			continue;
		}

		freeRegisters->insert(active_interval.physical_register);
	}

	*active = keep;
	std::sort(active->begin(), active->end(), IntervalEndComparator());
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
	std::vector<Interval> active;

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
			SpillAtInterval(&active, &interval);
		}
		else {
			PhysicalRegisterName* preg = *(freeRegisters.begin());
			freeRegisters.erase(preg);

			interval.physical_register = preg;
			active.push_back(interval);
			std::sort(active.begin(), active.end(), IntervalEndComparator());
		}
	}

	for (Interval& interval : intervals) {
		context->Allocated[interval.virtual_register] = interval.physical_register;
	}
}

/*********************************************************************************************************************/
