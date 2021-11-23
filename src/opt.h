#pragma once

namespace Helix
{
	namespace Options
	{
		bool GetDisableTerminalColouring();
		bool GetDebugAnnotateIR();

		void Parse(int argc, const char** argv);
	}
}