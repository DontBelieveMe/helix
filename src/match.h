/**
 * @file match.h
 * @author Barney Wilks
 */

#pragma once

#include "pass-manager.h"

/*********************************************************************************************************************/

namespace Helix
{
	class MachineExpander : public Pass
	{
	public:
		virtual void Execute(Module* mod, const PassRunInformation& info) override;
	};
}

/*********************************************************************************************************************/

REGISTER_PASS(MachineExpander, match, "[ARM] Matches the IR to it's machine instructions and generates machine IR");

/*********************************************************************************************************************/
