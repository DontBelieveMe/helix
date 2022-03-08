/**
 * @file scp.h
 * @author Barney Wilks
 *
 * Implementation of Kildall’s Simple Constant (SC) algorithm for constant
 * propagation
 *
 * (as described in Constant Propagation with Conditional Branches - MARK N. WEGMAN and F. KENNETH ZADECK)
 */

#pragma once

#include "pass-manager.h"

/*********************************************************************************************************************/

namespace Helix
{
	class SCP : public FunctionPass
	{
	public:
		void Execute(Function* fn, const PassRunInformation& info) override;
	};
}

REGISTER_PASS(SCP, scp, "[Generic] (Simple) Constant Propagation");

/*********************************************************************************************************************/
