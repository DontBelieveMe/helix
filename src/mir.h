/**
 * @file mir.h
 * @author Barney Wilks
 *
 * mir stands for Machine IR (aka low IR). This is a lower form of the abstract IR
 * that most of the compiler uses and exists mostly at the very end of the pipeline.
 * 
 * In Machine IR, each instruction represents an actual machine instruction (as defined
 * in the machine description file, such as arm.md).
 */

#pragma once

#include "instructions.h"

#include "arm-md.h" /* Generated */

namespace Helix
{
	class MachineInstruction : public Instruction
	{
	public:
		MachineInstruction(OpcodeType opcode, size_t nOperands)
			: Instruction(opcode, nOperands) { }
	};

	bool IsMachineTerminator(OpcodeType opc);
}
