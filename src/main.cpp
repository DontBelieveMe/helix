#include "frontend.h"
#include "helix.h"

int main(int argc, const char** argv)
{
	Helix::Initialise();
	Helix::Options::Parse(argc, argv);

	Helix::Frontend::Initialise();
	Helix::Frontend::Run(argc, argv);
	Helix::Frontend::Shutdown();

	Helix::Shutdown();

	HELIX_PROFILE_END
	return 0;
}
