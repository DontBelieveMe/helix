/**
 * @file test-stack-frame.cpp
 * @author Barney Wilks
 */

 /* Helix Core Includes */
#include "..\stack-frame.h"

/* Testing Library Includes */
#include "catch.hpp"

#pragma optimize("", off)

using namespace Helix;

/*********************************************************************************************************************/

TEST_CASE("Stack size aligns correctly", "[StackFrame]")
{
	SECTION("From a single word")
	{
		StackFrame stack;

		stack.Add(4);

		REQUIRE(stack.GetSizeAligned(1) == 4);
		REQUIRE(stack.GetSizeAligned(4) == 4);
		REQUIRE(stack.GetSizeAligned(8) == 8);
	}

	SECTION("From two words")
	{
		StackFrame stack;

		stack.Add(4); stack.Add(4);

		REQUIRE(stack.GetSizeAligned(1) == 8);
		REQUIRE(stack.GetSizeAligned(4) == 8);
		REQUIRE(stack.GetSizeAligned(8) == 8);
	}

	SECTION("From a word and a byte")
	{
		StackFrame stack;

		stack.Add(4);
		stack.Add(1);

		REQUIRE(stack.GetSizeAligned(1) == 5);
		REQUIRE(stack.GetSizeAligned(4) == 8);
		REQUIRE(stack.GetSizeAligned(8) == 8);
	}
}

/*********************************************************************************************************************/

TEST_CASE("Stack allows the addition of a single word", "[StackFrame]")
{
	StackFrame stack;
	
	const StackFrame::SlotIndex slot = stack.Add(4);

	REQUIRE(stack.GetAllocationOffset(slot) == 0);
	REQUIRE(stack.GetAllocationSize(slot) == 4);
}

/*********************************************************************************************************************/

TEST_CASE("Stack allows the addition of two words", "[StackFrame]")
{
	StackFrame stack;

	const StackFrame::SlotIndex slot0 = stack.Add(4);
	const StackFrame::SlotIndex slot1 = stack.Add(4);

	REQUIRE(stack.GetAllocationOffset(slot0) == 0);
	REQUIRE(stack.GetAllocationOffset(slot1) == 4);

	REQUIRE(stack.GetAllocationSize(slot0) == 4);
	REQUIRE(stack.GetAllocationSize(slot1) == 4);
}

/*********************************************************************************************************************/
