/**
 * @file match.h
 * @author Barney Wilks
 */

#pragma once

/* Internal Project Includes */
#include "pass-manager.h"

/*****************************************************************************/

namespace Helix
{
	class MachineExpander : public FunctionPass
	{
	public:
		virtual void Execute(Function* fn,
		                     const PassRunInformation& info) override;
	};
}

/******************************************************************************/

REGISTER_PASS(MachineExpander, match, "[ARM] Matches the IR to it's machine "
                                      "instructions and generates machine IR");

/*****************************************************************************/
