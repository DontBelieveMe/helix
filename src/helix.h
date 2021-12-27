#pragma once

#include "system.h"
#include "instructions.h"
#include "function.h"
#include "basic-block.h"
#include "types.h"
#include "value.h"
#include "print.h"
#include "options.h"
#include "module.h"
#include "pass-manager.h"

#define HELIX_VERSION  "1.0.0"
#define HELIX_APP_NAME "Helix C Compiler"

namespace Helix
{
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void Initialise();
	void Shutdown();

	std::string GetOutputFilePath(Module* module, const char* suffix);
	std::string GetAssemblyOutputFilePath(Module* module);

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}