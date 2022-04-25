/**
 * @file linear-scan.h
 * @author Barney Wilks
 *
 * Implements linear scan register allocation
 */

#pragma once

/* Internal Project Includes */
#include "interval.h"
#include "stack-frame.h"

/* C++ Standard Library Includes */
#include <unordered_map>

namespace Helix
{
	class VirtualRegisterName;
	class PhysicalRegisterName;

	/// Linear Scan Register Allocation
	namespace LSRA
	{
		struct Allocation
		{
			PhysicalRegisterName* Register;
			StackFrame::SlotIndex StackSlot;
		};

		using IntervalMap   = std::unordered_map<VirtualRegisterName*, Interval>;
		using AllocationMap = std::unordered_map<VirtualRegisterName*, Allocation>;

		struct Context
		{
			/// (Input) Map of variable (VirtualRegisterName*) to intervals,
			/// calculated in a previous step.
			IntervalMap InputIntervals;

			/// (Output) After allocation has been performed this will be
			/// a map of variables (VirtualRegisterName*) to the physical
			/// register allocated to it.
			AllocationMap Allocations;

			/// The stack, spilled variables will have a stack slot
			/// allocated from here
			StackFrame* Stack = nullptr;

			Context(const IntervalMap& intervals, StackFrame* stack);
			Context() = default;
		};

		void Run(Context* context);
	}
}
