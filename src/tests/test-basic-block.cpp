/**
 * @file test-basic-block.cpp
 * @author Barney Wilks
 */

 /* Helix Core Includes */
#include "..\basic-block.h"
#include "..\mir.h"

/* Testing Library Includes */
#include "catch.hpp"

using namespace Helix;

/*********************************************************************************************************************/

TEST_CASE("BasicBlock::Append", "[BasicBlock]")
{
	BasicBlock* bb = BasicBlock::Create();

	RetInsn* ret = Helix::CreateRet();
	bb->Append(ret);

	REQUIRE(bb->GetCountInstructions() == 1);
	REQUIRE(bb->GetLast() == ret);
}

/*********************************************************************************************************************/

TEST_CASE("BasicBlock::HasTerminator with HLIR terminator", "[BasicBlock]")
{
	BasicBlock* bb = BasicBlock::Create();

	bb->Append(Helix::CreateRet());

	REQUIRE(bb->HasTerminator());
}

/*********************************************************************************************************************/

TEST_CASE("BasicBlock::HasTerminator with LLIR terminator", "[BasicBlock]")
{
	BasicBlock* bb = BasicBlock::Create();
	bb->Append(ARMv7::CreateRet());

	REQUIRE(bb->HasTerminator());
}

/*********************************************************************************************************************/

TEST_CASE("BasicBlock::HasTerminator with a last non terminator instruction", "[BasicBlock]")
{
	BasicBlock* bb = BasicBlock::Create();
	bb->Append(Helix::CreateBinOp(HLIR::IAdd, nullptr, nullptr, nullptr));

	REQUIRE(!bb->HasTerminator());
}

/*********************************************************************************************************************/

TEST_CASE("BasicBlock::HasTerminator with no instructions in the block", "[BasicBlock]")
{
	BasicBlock* bb = BasicBlock::Create();
	REQUIRE(!bb->HasTerminator());
}

/*********************************************************************************************************************/

TEST_CASE("BasicBlock::IsEmpty when the block has no instructions", "[BasicBlock]")
{
	BasicBlock* bb = BasicBlock::Create();

	REQUIRE(bb->IsEmpty());
}

/*********************************************************************************************************************/

TEST_CASE("BasicBlock::IsEmpty when the block is not empty", "[BasicBlock]")
{
	BasicBlock* bb = BasicBlock::Create();
	bb->Append(Helix::CreateRet());

	REQUIRE(!bb->IsEmpty());
}

/*********************************************************************************************************************/

TEST_CASE("BasicBlock::IsEmpty when all instructions have been removed from the block", "[BasicBlock]")
{
	BasicBlock* bb = BasicBlock::Create();
	bb->Append(Helix::CreateRet());
	bb->Remove(bb->begin());

	REQUIRE(bb->IsEmpty());
}

/*********************************************************************************************************************/
