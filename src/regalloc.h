#pragma once

#include "pass-manager.h"

namespace Helix
{
	class BasicBlock;

	class RegisterAllocator : public BasicBlockPass
	{
	public:
		virtual void Execute(BasicBlock* bb) override;
	};

	REGISTER_PASS(RegisterAllocator, "regalloc", "Register allocation");
}