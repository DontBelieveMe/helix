#include "frontend.h"
#include "helix.h"

int main(int argc, const char** argv)
{
	using namespace Helix;

	Initialise();
	Frontend::Initialise();
	Module* tu = Frontend::Run(argc, argv);
	Frontend::Shutdown();

	if (!tu) {
		Shutdown();
		HELIX_PROFILE_END;
		return 1;
	}

	if (Options::GetEmitIR()) {
		Helix::DebugDump(*tu);
		Shutdown();
		return 0;
	}

	PassManager passManager;
	passManager.Execute(tu);

	Helix::DebugDump(*tu);

	Shutdown();
	HELIX_PROFILE_END
	return 0;
}
