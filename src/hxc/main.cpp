#include "../c/frontend.h"
#include "../profile.h"
#include "../helix.h"
#include "../pass-manager.h"

int main(int argc, const char** argv)
{
	Helix::Initialise();

	Helix::Frontend::Initialise();
	Helix::Module* pModule = Helix::Frontend::Run(argc, argv);
	Helix::Frontend::Shutdown();

	bool bError = false;

	if (pModule) {
		Helix::PassManager passManager;
		passManager.Execute(pModule);
	}
	else {
		bError = true;
	}

	HELIX_PROFILE_END;

	return bError ? 1 : 0;
}