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

	PassManager passManager;
	passManager.Execute(tu);

	Shutdown();
	HELIX_PROFILE_END
	return 0;
}
