#include "frontend.h"

int main(int argc, const char** argv)
{
	Helix::Frontend::Initialise();

	Helix::Frontend::Run(argc, argv);

	Helix::Frontend::Shutdown();

	return 0;
}
