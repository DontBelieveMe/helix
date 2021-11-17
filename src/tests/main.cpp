#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include "../types.h"

int main( int argc, char* argv[] ) {
	Helix::BuiltinTypes::Init();
	int result = Catch::Session().run( argc, argv );
	return result;
}
