/**
 * @file instruction-index.cpp
 * @author Barney Wilks
 *
 * Implementation of instruction-index.h
 */

/* Internal Project Includes */
#include "instruction-index.h"

using namespace Helix;

/*********************************************************************************************************************/

InstructionIndex::InstructionIndex(size_t block, size_t instruction)
	: block_index(block), instruction_index(instruction)
{ }

/*********************************************************************************************************************/

bool InstructionIndex::operator==(const InstructionIndex& other) const
{
	return block_index == other.block_index && instruction_index == other.instruction_index;
}

/*********************************************************************************************************************/

bool InstructionIndex::operator!=(const InstructionIndex& other) const
{
	return !operator==(other);
}

/*********************************************************************************************************************/

bool InstructionIndex::operator<(const InstructionIndex& other) const
{
	// If the block indices are the same then we actually need
	// to evaluate the instruction indices within that block
	// to see which comes first.
	if (block_index == other.block_index)
		return instruction_index < other.instruction_index;

	// Otherwise we can just compare the block indices to see
	// which comes first
	return block_index < other.block_index;
}

/*********************************************************************************************************************/

bool InstructionIndex::operator<=(const InstructionIndex& other) const
{
	return operator==(other) || operator<(other);
}

/*********************************************************************************************************************/

bool InstructionIndex::operator>(const InstructionIndex& other) const
{
	return !operator<(other) && !operator==(other);
}

/*********************************************************************************************************************/

bool InstructionIndex::operator>=(const InstructionIndex& other) const
{
	return operator==(other) || operator>(other);
}

/*********************************************************************************************************************/
