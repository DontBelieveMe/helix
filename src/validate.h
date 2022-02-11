/**
 * @file validate.h
 * @author Barney Wilks
 */

#pragma once

#include "pass-manager.h"

namespace Helix {
	class ValidationPass : public Pass {
	public:
		void Execute(Module* module, const PassRunInformation& info);
	};
}

REGISTER_PASS(ValidationPass, validate, "[Generic] Check that the IR is well formed (not nessesarily correct)");
