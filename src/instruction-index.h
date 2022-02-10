/**
 * @file instruction-index.h
 * @author Barney Wilks
 *
 * Allows uniquely identifying an instruction in a function
 * via a block/instruction index pair.
 * 
 * Important for linear scan register allocation.
 */

#pragma once

/* C Standard Library Includes */
#include <stdint.h>

namespace Helix
{
	struct InstructionIndex
	{
		size_t block_index       = SIZE_MAX;
		size_t instruction_index = SIZE_MAX;

		InstructionIndex(size_t block, size_t instruction);
		InstructionIndex() = default;

		bool operator==(const InstructionIndex& other) const;
		bool operator!=(const InstructionIndex& other) const;
		bool operator<(const InstructionIndex& other) const;
		bool operator<=(const InstructionIndex& other) const;
		bool operator>(const InstructionIndex& other) const;
		bool operator>=(const InstructionIndex& other) const;
	};
}