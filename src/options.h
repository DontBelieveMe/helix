#pragma once

#include <string>

namespace Helix
{
	namespace Options
	{
		#define ARGUMENT(type,default,varName,cliName,desc) type Get##varName();
			#include "options.def"

		void Parse(int argc, const char** argv);
	}
}