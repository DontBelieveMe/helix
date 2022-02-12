/**
 * @file test-linear-scan.cpp
 * @author Barney Wilks
 */

 /* Helix Core Includes */
#include "..\linear-scan.h"
#include "..\target-info-armv7.h"
#include "..\value.h"
#include "..\system.h"
#include "..\instructions.h"
#include "..\print.h"

/* Testing Library Includes */
#include "catch.hpp"

#pragma optimize("", off)

using namespace Helix;

/*********************************************************************************************************************/

/**
 * Verify that the same register doesn't get assigned to any overlapping intervals.
 */
bool CheckRegisterAllocationCorrectness(LSRA::Context& context)
{
	for (auto& [vreg, interval] : context.InputIntervals) {
		if (!PhysicalRegisters::IsValidPhysicalRegister(context.Allocated[vreg]))
			return false;

		for (const auto& [vreg2, interval2] : context.InputIntervals) {
			if (vreg == vreg2)
				continue;

			if (interval2.start >= interval.start && interval2.start < interval.end) {
				if (context.Allocated[vreg] == context.Allocated[vreg2])
					return false;
			}
		}
	}

	return true;
}

/*********************************************************************************************************************/

TEST_CASE("Simple allocation (less vregs than available registers)", "[LRSA]")
{
	VirtualRegisterName* a = VirtualRegisterName::Create(BuiltinTypes::GetInt32());
	VirtualRegisterName* b = VirtualRegisterName::Create(BuiltinTypes::GetInt32());

	LSRA::Context context;
	context.InputIntervals[a] = Interval(a, InstructionIndex(0, 2), InstructionIndex(0, 5));
	context.InputIntervals[b] = Interval(b, InstructionIndex(0, 1), InstructionIndex(0, 3));

	LSRA::Run(&context);

	// Should both be assigned to physical registers
	REQUIRE(PhysicalRegisters::IsValidPhysicalRegister(context.Allocated[a]));
	REQUIRE(PhysicalRegisters::IsValidPhysicalRegister(context.Allocated[b]));

	// And shouldn't be assigned to the same register
	REQUIRE(context.Allocated[a] != context.Allocated[b]);

	REQUIRE(CheckRegisterAllocationCorrectness(context));
}

/*********************************************************************************************************************/

TEST_CASE("Simple allocation (more vregs than available registers, but lifetimes don't overlap)", "[LRSA]")
{
	std::vector<VirtualRegisterName*> vregs;
	LSRA::Context                     context;

	for (size_t i = 0; i < 16; ++i) {
		VirtualRegisterName* v = VirtualRegisterName::Create(BuiltinTypes::GetInt32());
		context.InputIntervals[v] = Interval(v, InstructionIndex(0, i * 3), InstructionIndex(0, (i * 3) + 2));
		vregs.push_back(v);
	}

	LSRA::Run(&context);

	PhysicalRegisterName* first = context.Allocated[vregs[0]];

	for (const auto& [virtualRegister, physicalRegister] : context.Allocated) {
		REQUIRE(PhysicalRegisters::IsValidPhysicalRegister(physicalRegister));

		// In the register allocator is free to assign any register to any of the
		// intervals (since none overlap, any register is available)
		//
		// However in practice it will assign them all to the same register &
		// I want to check that it does that.
		//
		// Technically this isn't a proper unit test (blah, blah don't test implementation details
		// bleugh).
		REQUIRE(physicalRegister == first);
	}

	REQUIRE(CheckRegisterAllocationCorrectness(context));
}

/*********************************************************************************************************************/

TEST_CASE("Simple allocation (more vregs than available registers, but some lifetimes overlap, no spill)", "[LRSA]")
{
	std::vector<VirtualRegisterName*> vregs;
	LSRA::Context                     context;

	int start = 0;
	int end = 4;

	for (size_t i = 0; i < 16; ++i) {
		VirtualRegisterName* v = VirtualRegisterName::Create(BuiltinTypes::GetInt32());
		context.InputIntervals[v] = Interval(v, InstructionIndex(0, start), InstructionIndex(0, end));
		vregs.push_back(v);

		start += 2; end += 2;
	}

	LSRA::Run(&context);

	// Test that all intervals have been assigned a valid register
	for (const auto& [virtualRegister, physicalRegister] : context.Allocated) {
		REQUIRE(PhysicalRegisters::IsValidPhysicalRegister(physicalRegister));
	}
	
	REQUIRE(CheckRegisterAllocationCorrectness(context));
}

/*********************************************************************************************************************/

TEST_CASE("Test CheckRegisterAllocationCorrectness utility function", "[LRSA]")
{
	VirtualRegisterName* a = VirtualRegisterName::Create(BuiltinTypes::GetInt32());
	VirtualRegisterName* b = VirtualRegisterName::Create(BuiltinTypes::GetInt32());

	SECTION("Overlapping range assigned to the same register, failure") {
		LSRA::Context context;

		context.InputIntervals[a] = Interval(a, InstructionIndex(0, 2), InstructionIndex(0, 5));
		context.InputIntervals[b] = Interval(b, InstructionIndex(0, 3), InstructionIndex(0, 6));

		context.Allocated[a] = PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::R5);
		context.Allocated[b] = PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::R5);

		REQUIRE(!CheckRegisterAllocationCorrectness(context));
	}

	SECTION("Overlapping range assigned to different registers, success") {
		LSRA::Context context;

		context.InputIntervals[a] = Interval(a, InstructionIndex(0, 2), InstructionIndex(0, 5));
		context.InputIntervals[b] = Interval(b, InstructionIndex(0, 3), InstructionIndex(0, 6));

		context.Allocated[a] = PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::R5);
		context.Allocated[b] = PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::R6);

		REQUIRE(CheckRegisterAllocationCorrectness(context));
	}

	SECTION("Non overlapping ranges, success") {
		LSRA::Context context;

		context.InputIntervals[a] = Interval(a, InstructionIndex(0, 2), InstructionIndex(0, 5));
		context.InputIntervals[b] = Interval(b, InstructionIndex(0, 5), InstructionIndex(0, 20));

		context.Allocated[a] = PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::R5);
		context.Allocated[b] = PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::R5);

		REQUIRE(CheckRegisterAllocationCorrectness(context));
	}
}

/*********************************************************************************************************************/
