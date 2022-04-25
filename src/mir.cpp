/**
 * @file mir.cpp
 * @author Barney Wilks
 *
 * Implementation of mir.h
 */

// Internal Project Includes
#include "mir.h"

using namespace Helix;

/******************************************************************************/

MachineInstruction::MachineInstruction(OpcodeType opcode,
                                       size_t nOperands)
	: Instruction(opcode, nOperands)
{ }

/******************************************************************************/

Instruction::OperandFlags
MachineInstruction::GetOperandFlags(size_t index) const
{
	helix_assert(index < 4,
	             "didn't expect machine operand to have so many operands...");

	if (index >= 4)
		return OP_NONE;

	return Flags[index];
}

/******************************************************************************/

void
MachineInstruction::SetOperandFlag(size_t index, OperandFlags flags)
{
	helix_assert(index < 4,
	             "didn't expect machine operand to have so many operands...");

	if (index >= 4)
		return;

	Flags[index] = flags;
}

/******************************************************************************/

bool
Helix::IsMachineTerminator(OpcodeType opc)
{
	switch (opc) {
	case ARMv7::Bge:
	case ARMv7::Bgt:
	case ARMv7::Blt:
	case ARMv7::Ble:
	case ARMv7::Beq:
	case ARMv7::Bne:
	case ARMv7::Br:
	case ARMv7::Ret:
		return true;

	default:
		return false;
	}
}

/******************************************************************************/
