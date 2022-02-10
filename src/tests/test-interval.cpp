/**
 * @file test-interval.h
 * @author Barney Wilks
 */

 /* Helix Core Includes */
#include "../interval.h"
#include "../instructions.h"

/* Testing Library Includes */
#include "catch.hpp"

using namespace Helix;

/*********************************************************************************************************************/

TEST_CASE("Default constructor sets appropate defaults", "[Interval]")
{
	Helix::Interval interval;

	REQUIRE(interval.virtual_register == nullptr);
	REQUIRE(interval.physical_register == nullptr);
	REQUIRE(interval.stack_slot == SIZE_MAX);
}

/*********************************************************************************************************************/

TEST_CASE("Two intervals representing the same range are equal", "[Interval]")
{
	Helix::Interval a;
	a.start = InstructionIndex(4, 5);
	a.end   = InstructionIndex(4, 8);
	a.stack_slot = 123;

	Helix::Interval b;
	b.start = InstructionIndex(4, 5);
	b.end   = InstructionIndex(4, 8);
	b.stack_slot = 512;

	REQUIRE(a == b);
}

/*********************************************************************************************************************/

TEST_CASE("Interval start comparator works", "[IntervalStartComparator]")
{
	Helix::IntervalStartComparator comparator;

	SECTION("first interval does start first (same block)")
	{
		Helix::Interval a;
		a.start = InstructionIndex(4, 1);
		a.end = InstructionIndex(4, 8);

		Helix::Interval b;
		b.start = InstructionIndex(4, 5);
		b.end = InstructionIndex(4, 8);

		REQUIRE(comparator(a, b));
	}

	SECTION("first interval starts first (different blocks)")
	{
		Helix::Interval a;
		a.start = InstructionIndex(2, 1);
		a.end = InstructionIndex(4, 8);

		Helix::Interval b;
		b.start = InstructionIndex(4, 5);
		b.end = InstructionIndex(4, 8);

		REQUIRE(comparator(a, b));
	}

	SECTION("intervals start at the same time")
	{
		Helix::Interval a;
		a.start = InstructionIndex(4, 5);
		a.end = InstructionIndex(4, 8);

		Helix::Interval b;
		b.start = InstructionIndex(4, 5);
		b.end = InstructionIndex(4, 8);

		REQUIRE(!comparator(a, b));
	}

	SECTION("second interval starts first")
	{
		Helix::Interval a;
		a.start = InstructionIndex(4, 5);
		a.end = InstructionIndex(4, 8);

		Helix::Interval b;
		b.start = InstructionIndex(3, 5);
		b.end = InstructionIndex(4, 8);

		REQUIRE(!comparator(a, b));
	}
}

/*********************************************************************************************************************/

TEST_CASE("Interval end comparator works", "[IntervalEndComparator]")
{
	Helix::IntervalEndComparator comparator;

	SECTION("first interval does end first (same block)")
	{
		Helix::Interval a;
		a.start = InstructionIndex(4, 1);
		a.end = InstructionIndex(4, 1);

		Helix::Interval b;
		b.start = InstructionIndex(4, 2);
		b.end = InstructionIndex(4, 3);

		REQUIRE(comparator(a, b));
	}

	SECTION("first interval ends first (different blocks)")
	{
		Helix::Interval a;
		a.start = InstructionIndex(2, 1);
		a.end = InstructionIndex(5, 8);

		Helix::Interval b;
		b.start = InstructionIndex(3, 3);
		b.end = InstructionIndex(6, 8);

		REQUIRE(comparator(a, b));
	}

	SECTION("intervals end at the same time")
	{
		Helix::Interval a;
		a.start = InstructionIndex(4, 5);
		a.end = InstructionIndex(4, 8);

		Helix::Interval b;
		b.start = InstructionIndex(4, 5);
		b.end = InstructionIndex(4, 8);

		REQUIRE(!comparator(a, b));
	}

	SECTION("second interval ends first")
	{
		Helix::Interval a;
		a.start = InstructionIndex(4, 5);
		a.end = InstructionIndex(4, 8);

		Helix::Interval b;
		b.start = InstructionIndex(3, 5);
		b.end = InstructionIndex(4, 3);

		REQUIRE(!comparator(a, b));
	}
}

/*********************************************************************************************************************/

TEST_CASE("Interval end/start comparator works", "[IntervalEndStartComparator]")
{
	Helix::IntervalEndStartComparator comparator;

	SECTION("first interval ends before second starts (same block)")
	{
		Helix::Interval a;
		a.start = InstructionIndex(4, 1);
		a.end = InstructionIndex(4, 1);

		Helix::Interval b;
		b.start = InstructionIndex(4, 3);
		b.end = InstructionIndex(4, 5);

		REQUIRE(comparator(a, b));
	}

	SECTION("first interval ends before second starts (different blocks)")
	{
		Helix::Interval a;
		a.start = InstructionIndex(2, 0);
		a.end = InstructionIndex(5, 10);

		Helix::Interval b;
		b.start = InstructionIndex(6, 0);
		b.end = InstructionIndex(7, 0);

		REQUIRE(comparator(a, b));
	}

	SECTION("first interval ends as second starts (same time)")
	{
		Helix::Interval a;
		a.start = InstructionIndex(4, 5);
		a.end = InstructionIndex(4, 8);

		Helix::Interval b;
		b.start = InstructionIndex(4, 8);
		b.end = InstructionIndex(4, 10);

		REQUIRE(!comparator(a, b));
	}

	SECTION("second interval starts before first interval ends")
	{
		Helix::Interval a;
		a.start = InstructionIndex(4, 0);
		a.end = InstructionIndex(5, 0);

		Helix::Interval b;
		b.start = InstructionIndex(3, 5);
		b.end = InstructionIndex(10, 10);

		REQUIRE(!comparator(a, b));
	}
}

/*********************************************************************************************************************/
