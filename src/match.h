#pragma once

#include "pass-manager.h"

namespace Helix
{
	class FinalMatcher : public Pass
	{
	public:
		virtual void Execute(Module* mod) override;

	private:
	};

	REGISTER_PASS(FinalMatcher, "final", "Final pass, matches the IR to assembly and emits it to file");
}