/**
 * @file test-ir-helpers.cpp
 * @author Barney Wilks
 */

 /* Helix Core Includes */
#include "..\ir-helpers.h"

/* Testing Library Includes */
#include "catch.hpp"

using namespace Helix;

/******************************************************************************/

TEST_CASE("IR::InsnSeq (Empty)", "[InsnSeq]")
{
	IR::InsnSeq seq;

	REQUIRE(seq.GetSize() == 0);
	REQUIRE(seq.GetHead() == nullptr);
	REQUIRE(seq.GetTail() == nullptr);
}

/******************************************************************************/

TEST_CASE("IR::InsnSeq (Single Instruction)", "[InsnSeq]")
{
	RetInsn* ret = Helix::CreateRet();

	IR::InsnSeq seq;
	seq.Append(ret);

	REQUIRE(seq.GetSize() == 1);
	REQUIRE(seq.GetHead() == ret);
	REQUIRE(seq.GetTail() == ret);
}

/******************************************************************************/

TEST_CASE("IR::InsnSeq (Two Instructions)", "[InsnSeq]")
{
	RetInsn* ret_a = Helix::CreateRet();
	RetInsn* ret_b = Helix::CreateRet();

	IR::InsnSeq seq;

	seq.Append(ret_a);
	seq.Append(ret_b);

	REQUIRE(seq.GetSize() == 2);
	REQUIRE(seq.GetHead() == ret_a);
	REQUIRE(seq.GetTail() == ret_b);
}

/******************************************************************************/

TEST_CASE("IR::InsnSeq begin/end iteration", "[InsnSeq]")
{
	RetInsn* ret_a = Helix::CreateRet();
	RetInsn* ret_b = Helix::CreateRet();

	IR::InsnSeq seq;

	for (auto it = seq.begin(); it != seq.end(); ++it)
		REQUIRE(false);

	seq.Append(ret_a);

	for (auto it = seq.begin(); it != seq.end(); ++it)
		REQUIRE(&(*it) == ret_a);

	seq.Append(ret_b);

	RetInsn* insns[] = { ret_a, ret_b };
	size_t i = 0;
	for (auto it = seq.begin(); it != seq.end(); ++it, i++)
		REQUIRE(&(*it) == insns[i]);
}

/******************************************************************************/

TEST_CASE("IR::ReplaceInstructionAndDestroyOriginal (Seq Size = 1, BB Size = 1)",
		"[InsnSeq]")
{
	BasicBlock* bb = BasicBlock::Create();
	RetInsn* original = Helix::CreateRet();
	RetInsn* replacement = Helix::CreateRet();

	bb->Append(original);

	IR::InsnSeq seq;
	seq.Append(replacement);

	IR::ReplaceInstructionAndDestroyOriginal(original, seq);

	REQUIRE(bb->GetCountInstructions() == 1);
	for (auto it = bb->begin(); it != bb->end(); ++it) {
		Instruction* insn = &(*it);

		REQUIRE(insn->GetParent() == bb);
		REQUIRE(insn == replacement);
	}
}

/******************************************************************************/

TEST_CASE("IR::ReplaceInstructionAndDestroyOriginal (Seq Size = n, BB Size = 1)",
		"[InsnSeq]")
{
	BasicBlock* bb = BasicBlock::Create();
	RetInsn* original = Helix::CreateRet();

	RetInsn* replacements[] = {
		Helix::CreateRet(),
		Helix::CreateRet()
	};

	bb->Append(original);

	IR::InsnSeq seq;
	seq.Append(replacements[0]);
	seq.Append(replacements[1]);

	IR::ReplaceInstructionAndDestroyOriginal(original, seq);

	REQUIRE(bb->GetCountInstructions() == 2);

	size_t i = 0;
	for (auto it = bb->begin(); it != bb->end(); ++it, i++) {
		Instruction* insn = &(*it);

		REQUIRE(insn->GetParent() == bb);
		REQUIRE(insn == replacements[i]);
	}
}

/******************************************************************************/

TEST_CASE("IR::ReplaceInstructionAndDestroyOriginal (Seq Size = 1, BB Size = n)",
		"[InsnSeq]")
{
	BasicBlock* bb = BasicBlock::Create();

	RetInsn* original_1 = Helix::CreateRet();
	RetInsn* original_2 = Helix::CreateRet();
	RetInsn* original_3 = Helix::CreateRet();

	bb->Append(original_1);
	bb->Append(original_2);
	bb->Append(original_3);

	RetInsn* replacement = Helix::CreateRet();

	IR::InsnSeq seq;
	seq.Append(replacement);

	IR::ReplaceInstructionAndDestroyOriginal(original_2, seq);

	REQUIRE(bb->GetCountInstructions() == 3);

	RetInsn* expected[] = { original_1, replacement, original_3 };
	size_t i = 0;

	for (auto it = bb->begin(); it != bb->end(); ++it, i++) {
		Instruction* insn = &(*it);

		REQUIRE(insn->GetParent() == bb);
		REQUIRE(insn == expected[i]);
	}
}

/******************************************************************************/

TEST_CASE("IR::ReplaceInstructionAndDestroyOriginal (Seq Size = n, BB Size = n)",
		"[InsnSeq]")
{
	BasicBlock* bb = BasicBlock::Create();

	RetInsn* original_1 = Helix::CreateRet();
	RetInsn* original_2 = Helix::CreateRet();
	RetInsn* original_3 = Helix::CreateRet();

	bb->Append(original_1);
	bb->Append(original_2);
	bb->Append(original_3);

	RetInsn* replacement_1 = Helix::CreateRet();
	RetInsn* replacement_2 = Helix::CreateRet();

	IR::InsnSeq seq;
	seq.Append(replacement_1);
	seq.Append(replacement_2);

	IR::ReplaceInstructionAndDestroyOriginal(original_2, seq);

	REQUIRE(bb->GetCountInstructions() == 4);

	RetInsn* expected[] = { original_1, replacement_1,
	                        replacement_2, original_3 };
	size_t i = 0;

	for (auto it = bb->begin(); it != bb->end(); ++it, i++) {
		Instruction* insn = &(*it);

		REQUIRE(insn->GetParent() == bb);
		REQUIRE(insn == expected[i]);
	}
}

/******************************************************************************/

TEST_CASE("IR::ReplaceInstructionAndDestroyOriginal (Seq Size = 0, BB Size = n)",
		"[InsnSeq]")
{
	BasicBlock* bb = BasicBlock::Create();

	RetInsn* original_1 = Helix::CreateRet();
	RetInsn* original_2 = Helix::CreateRet();
	RetInsn* original_3 = Helix::CreateRet();

	bb->Append(original_1);
	bb->Append(original_2);
	bb->Append(original_3);

	IR::InsnSeq seq;

	IR::ReplaceInstructionAndDestroyOriginal(original_2, seq);

	REQUIRE(bb->GetCountInstructions() == 2);

	RetInsn* expected[] = { original_1, original_3 };
	size_t i = 0;

	for (auto it = bb->begin(); it != bb->end(); ++it, i++) {
		Instruction* insn = &(*it);

		REQUIRE(insn->GetParent() == bb);
		REQUIRE(insn == expected[i]);
	}
}

/******************************************************************************/

