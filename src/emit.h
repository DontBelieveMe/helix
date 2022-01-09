/**
 * @file emit.h
 * @author Barney Wilks
 * 
 * Pass implementing the final code generation phase - doing pattern matching
 * aginst the input IR and producing the equivilant assembly.
 * 
 * See comments in match.cpp for implementation details.
 */

#pragma once

#include "pass-manager.h"

/*********************************************************************************************************************/

namespace Helix
{
	class AssemblyEmitter : public Pass
	{
	public:
		virtual void Execute(Module* mod) override;
	};
}

/*********************************************************************************************************************/

REGISTER_PASS(AssemblyEmitter, emit, "[ARM] Matches the IR to it's machine instructions and emits it as assembly to a file");

/*********************************************************************************************************************/
