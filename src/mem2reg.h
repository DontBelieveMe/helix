/**
 * @file mem2reg.h
 * @author Barney Wilks
 */

#pragma once

#include "pass-manager.h"

/*********************************************************************************************************************/

namespace Helix
{
	class Mem2Reg : public FunctionPass
	{
	public:
		void Execute(Function* fn, const PassRunInformation& info) override;

	private:
	};
}

REGISTER_PASS(Mem2Reg, mem2reg, "[Generic] Promotes memory references to be register references");

/*********************************************************************************************************************/
