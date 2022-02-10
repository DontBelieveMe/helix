/**
 * @file test-instruction-index.h
 * @author Barney Wilks
 */

/* Helix Core Includes */
#include "../instruction-index.h"

/* Testing Library Includes */
#include "catch.hpp"

/*********************************************************************************************************************/

TEST_CASE("Constructor correctly sets block & instruction index", "[InstructionIndex]")
{
	SECTION("InstructionIndex::InstructionIndex()")
	{
		Helix::InstructionIndex index;

		REQUIRE(index.block_index == SIZE_MAX);
		REQUIRE(index.instruction_index == SIZE_MAX);
	}

	SECTION("InstructionIndex::InstructionIndex(size_t,size_t)")
	{
		Helix::InstructionIndex index(4, 3);

		REQUIRE(index.block_index == 4);
		REQUIRE(index.instruction_index == 3);
	}
}

/*********************************************************************************************************************/

TEST_CASE("Equals comparison works as expected", "[InstructionIndex]")
{
	Helix::InstructionIndex index(5, 3);

	SECTION("Correctly compares against identical instruction index")
	{
		Helix::InstructionIndex other(5, 3);

		REQUIRE(index == other);
	}

	SECTION("Doesn't match against different instruction index")
	{
		Helix::InstructionIndex other(34, 543);

		REQUIRE(!(index == other));
	}
}

/*********************************************************************************************************************/

TEST_CASE("Not equals comparison works as expected", "[InstructionIndex]")
{
	Helix::InstructionIndex index(3, 7);

	SECTION("Correctly doesn't match against different index")
	{
		Helix::InstructionIndex other(66, 554);

		REQUIRE(index != other);
	}

	SECTION("Doesn't match against identical index")
	{
		Helix::InstructionIndex other(3, 7);

		REQUIRE(!(index != other));
	}
}

/*********************************************************************************************************************/

TEST_CASE("Less than comparison works as expected", "[InstructionIndex]")
{
	Helix::InstructionIndex index(5, 3);

	SECTION("Positive Tests")
	{
		SECTION("Returns true when compared to a bigger index (1)")
		{
			Helix::InstructionIndex other(65, 3);

			REQUIRE(index < other);
		}

		SECTION("Returns true when compared to a bigger index (2)")
		{
			Helix::InstructionIndex other(5, 4);

			REQUIRE(index < other);
		}
	}

	SECTION("Negative Tests")
	{
		SECTION("Returns false when compared to a smaller index (1)")
		{
			Helix::InstructionIndex other(1, 3);

			REQUIRE(!(index < other));
		}

		SECTION("Returns false when compared to a smaller index (2)")
		{
			Helix::InstructionIndex other(5, 2);

			REQUIRE(!(index < other));
		}
	}
}

/*********************************************************************************************************************/
