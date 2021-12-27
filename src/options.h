#pragma once

#include <string>

namespace Helix
{
	namespace Options
	{
		#define ARGUMENT(type,default,varName,cliName,desc) type Get##varName();
		#define ARGUMENT_LIST(type,varName,cliName,desc) type Get##varName(size_t index); size_t GetCount##varName##s();
		#define ARGUMENTS_POSITIONAL(type, varName, desc) type Get##varName(size_t index); size_t GetCount##varName##s();
			#include "options.def"
	}
}