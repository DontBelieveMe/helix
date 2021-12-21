#pragma once

#include "pass-manager.h"

namespace Helix
{
	class BasicBlock;

	class RegisterAllocator : public FunctionPass
	{
	public:
		virtual void Execute(Function* fn) override;
	};

}

REGISTER_PASS(RegisterAllocator, regalloc, "[ARM] Register allocation");