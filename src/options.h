#pragma once

namespace Helix
{
	namespace Options
	{
		bool GetDisableTerminalColouring();
		bool GetDebugAnnotateIR();
		bool GetDisableLogging();

		void Parse(int argc, const char** argv);
	}
}