/**
 * @file test-mir.cpp
 * @author Barney Wilks
 */

 /* Helix Core Includes */
#include "..\mir.h"

/* Testing Library Includes */
#include "catch.hpp"

using namespace Helix;

/*********************************************************************************************************************/

TEST_CASE("IsMachineTerminator", "[MIR]")
{
	SECTION("Where given opcode is a terminator instruction")
	{
		REQUIRE(Helix::IsMachineTerminator(ARMv7::Beq));
		REQUIRE(Helix::IsMachineTerminator(ARMv7::Blt));
		REQUIRE(Helix::IsMachineTerminator(ARMv7::Bgt));
		REQUIRE(Helix::IsMachineTerminator(ARMv7::Bge));
		REQUIRE(Helix::IsMachineTerminator(ARMv7::Bgt));
		REQUIRE(Helix::IsMachineTerminator(ARMv7::Bne));
		REQUIRE(Helix::IsMachineTerminator(ARMv7::Ret));
		REQUIRE(Helix::IsMachineTerminator(ARMv7::Br));
	}

	SECTION("Where given opcode is not a terminator instruction (not a LLIR instruction)")
	{
		REQUIRE(!Helix::IsMachineTerminator(HLIR::ConditionalBranch));
		REQUIRE(!Helix::IsMachineTerminator(HLIR::UnconditionalBranch));
	}

	SECTION("Where given opcode is not a terminator instruction (another LLIR instruction)")
	{
		REQUIRE(!Helix::IsMachineTerminator(ARMv7::Movwge));
		REQUIRE(!Helix::IsMachineTerminator(ARMv7::Add_r32i32));
		REQUIRE(!Helix::IsMachineTerminator(ARMv7::Cmpi));
	}
}

/*********************************************************************************************************************/
