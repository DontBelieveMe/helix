/**
 * @file opcodes.h
 * @author Barney Wilks
 *
 * Contains high level IR opcode definitions & utility functions.
 * Distinct from low (aka machine) level IR, where instructions correspond more directly
 * to actual instructions instead.
 *
 * Actual instructions are defined in `insns.def`.
 */

#pragma once

/*********************************************************************************************************************/

#define IMPLEMENT_OPCODE_CATEGORY_IDENTITY(category) \
	constexpr inline bool Is##category(Helix::OpcodeType opc) \
	{ \
		return opc > kInsnStart_##category && opc < kInsnEnd_##category; \
	}

/*********************************************************************************************************************/

namespace Helix
{
	using OpcodeType = unsigned;

	static constexpr size_t MAX_COUNT_IR_INSNS = 1024;

	enum Opcode : OpcodeType
	{
		#define BEGIN_INSN_CLASS(class_name) kInsnStart_##class_name,
		#define END_INSN_CLASS(class_name) kInsnEnd_##class_name,
		#define DEF_INSN_FIXED(code_name, pretty_name,n_operands, ...) kInsn_##code_name,
		#define DEF_INSN_DYN(code_name, pretty_name) kInsn_##code_name,
			#include "insns.def"

		kInsnCount
	};

	#define BEGIN_INSN_CLASS(class_name) IMPLEMENT_OPCODE_CATEGORY_IDENTITY(class_name)
		#include "insns.def"

	/*********************************************************************************************************************/

	constexpr inline bool IsMachineOpcode(Helix::OpcodeType opc) { return opc >= MAX_COUNT_IR_INSNS; }

	/*********************************************************************************************************************/

	static_assert(kInsnCount < MAX_COUNT_IR_INSNS, "Too many IR instructions (must not exceed MAX_COUNT_IR_INSNS)");
}

/*********************************************************************************************************************/
