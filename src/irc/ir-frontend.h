#pragma once

namespace Helix { class Module; }

namespace Helix::Frontend::IR
{
	void Initialise();
	Module* Run(int argc, const char** argv);
	void Shutdown();
}
