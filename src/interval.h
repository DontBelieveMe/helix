/**
 * @file interval.h
 * @author Barney Wilks
 *
 * Defines a live interval - a coarse representation
 * of the lifetime of a variable (doesn't represent lifetime
 * holes etc...).
 * 
 * Critical foundation of linear scan register allocation
 */

#pragma once

/* Internal Project Includes */
#include "instruction-index.h"

namespace Helix
{
	class VirtualRegisterName;
	class PhysicalRegisterName;

	struct Interval
	{
		/// Start of the intervals range
		InstructionIndex start;

		/// End of the intervals range
		InstructionIndex end;

		/// The variable that this interval is representing the lifetime of
		VirtualRegisterName* virtual_register = nullptr;

		/// The physical register allocated to this range (after register allocation)
		PhysicalRegisterName* physical_register = nullptr;

		/// (After register allocation) if this interval couldn't be assigned a register
		/// the stack slot to spill to/from.
		size_t stack_slot = SIZE_MAX;

		Interval(VirtualRegisterName* variable);
		Interval() = default;

		bool operator==(const Interval& other) const;
	};

	/**
	 * A comparator that compares the starts
	 * of two intervals and sees which starts first
	 */
	struct IntervalStartComparator
	{
		/**
		 * Return true if interval 'a' starts before interval 'b' ends.
		 * Return false if interval 'b' starts before 'a' or they start
		 * at the same time.
		 */
		bool operator()(const Interval& a, const Interval& b) const;
	};

	/**
	 * A comparator that compares the ends of two intervals
	 * and sees which ends first.
	 */
	struct  IntervalEndComparator
	{
		/**
		 * Return true if interval 'a' ends before interval 'b' ends.
		 * Return false if interval 'b' ends before 'a' or they end
		 * at the same time.
		 */
		bool operator()(const Interval& a, const Interval& b) const;
	};

	/**
	 * A comparator which compares two intervals to see if
	 * the first ends before the second starts.
	 */
	struct IntervalEndStartComparator
	{
		/**
		 * Return true if interval 'a' ends before interval 'b' starts.
		 * Return false if interval 'b' overlaps with 'a'.
		 */
		bool operator()(const Interval& a, const Interval& b) const;
	};
}
