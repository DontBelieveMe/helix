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
}

REGISTER_PASS(FinalMatcher, match, "[ARM] Matches the IR to it's machine instructions and emits it as assembly to a file");