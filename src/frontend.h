#pragma once

namespace Helix { class Module; }

namespace Helix::Frontend
{
	void Initialise();
	Module* Run(int argc, const char** argv);
	void Shutdown();
}
