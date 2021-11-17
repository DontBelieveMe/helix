#include "helix.h"
#include "types.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Helix::Initialise()
{
	BuiltinTypes::Init();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Helix::Shutdown()
{
	BuiltinTypes::Destroy();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////