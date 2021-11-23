#include "opt.h"

#include <string.h>

static bool s_DisableTerminalColouring = true;
static bool s_DebugAnnotateIR = false;

void Helix::Options::Parse(int argc, const char** argv)
{
	for (int i = 1; i < argc; ++i) {
		const char* arg = argv[i];

		if (strcmp(arg, "-Hdisable-terminal-colouring") == 0)
			s_DisableTerminalColouring = true;
		else if (strcmp(arg, "-Hdebug-annotate-ir") == 0)
			s_DebugAnnotateIR = true;
	}
}


bool Helix::Options::GetDisableTerminalColouring() { return s_DisableTerminalColouring; }
bool Helix::Options::GetDebugAnnotateIR() { return s_DebugAnnotateIR; }
