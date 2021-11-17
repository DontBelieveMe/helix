#include "frontend.h"
#include "helix.h"

int main(int argc, const char** argv)
{
	Helix::Initialise();

	Helix::Frontend::Initialise();
	Helix::Frontend::Run(argc, argv);
	Helix::Frontend::Shutdown();

	Helix::Shutdown();

	return 0;
}
