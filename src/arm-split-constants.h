#pragma once

#include "pass-manager.h"

namespace Helix
{
	class ArmSplitConstants : public FunctionPass
	{
	public:
		void Execute(Function* fn, const PassRunInformation& info) override;

	private:
	};
}

REGISTER_PASS(ArmSplitConstants, armsplitconstants, "[ARM] Split constants into a form compatible with the ARM ISA");
