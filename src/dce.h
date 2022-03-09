/**
 * @file dce.h
 * @author Barney Wilks
 */

#pragma once

#include "pass-manager.h"

 /*********************************************************************************************************************/

namespace Helix
{
	class DCE : public FunctionPass
	{
	public:
		void Execute(Function* fn, const PassRunInformation& info) override;
	};
}

REGISTER_PASS(DCE, dce, "[Generic] Dead Code Elimination");

/*********************************************************************************************************************/
