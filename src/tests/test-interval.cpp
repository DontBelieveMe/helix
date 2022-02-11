/**
 * @file test-interval.h
 * @author Barney Wilks
 */

 /* Helix Core Includes */
#include "../interval.h"
#include "../instructions.h"
#include "../function.h"

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

TEST_CASE("Calculating intervals from function", "[ComputeIntervalsForFunction]")
{
	const FunctionType* functionType = FunctionType::Create(BuiltinTypes::GetInt32(), {});
	Function* fn = Function::Create(functionType, "main", {});

	BasicBlock* bb = BasicBlock::Create();

	VirtualRegisterName* a = VirtualRegisterName::Create(BuiltinTypes::GetInt32());
	VirtualRegisterName* b = VirtualRegisterName::Create(BuiltinTypes::GetInt32());
	VirtualRegisterName* c = VirtualRegisterName::Create(BuiltinTypes::GetInt32());
	VirtualRegisterName* d = VirtualRegisterName::Create(BuiltinTypes::GetInt32());

	ConstantInt* one = ConstantInt::Create(BuiltinTypes::GetInt32(), 1);
	ConstantInt* two = ConstantInt::Create(BuiltinTypes::GetInt32(), 2);
	ConstantInt* three = ConstantInt::Create(BuiltinTypes::GetInt32(), 3);

	bb->Append(Helix::CreateBinOp(HLIR::IAdd, one, two,   a)); // (1) a = 1 + 2;
	bb->Append(Helix::CreateBinOp(HLIR::IMul, a,   two,   b)); // (2) b = a * 2;
	bb->Append(Helix::CreateBinOp(HLIR::IAdd, two, three, c)); // (3) c = 2 + 3;
	bb->Append(Helix::CreateBinOp(HLIR::ISub, b,   c,     d)); // (4) d = b - c;
	bb->Append(Helix::CreateRet(d));                           // (5) return d;

	fn->Append(bb);
	fn->RunLivenessAnalysis();

	std::unordered_map<VirtualRegisterName*, Interval> intervals;
	Helix::ComputeIntervalsForFunction(fn, intervals);

	REQUIRE(intervals[a] == Interval(a, InstructionIndex(0, 0), InstructionIndex(0, 1)));
	REQUIRE(intervals[b] == Interval(b, InstructionIndex(0, 1), InstructionIndex(0, 3)));
	REQUIRE(intervals[c] == Interval(c, InstructionIndex(0, 2), InstructionIndex(0, 3)));
	REQUIRE(intervals[d] == Interval(c, InstructionIndex(0, 3), InstructionIndex(0, 4)));
}


/*********************************************************************************************************************/
