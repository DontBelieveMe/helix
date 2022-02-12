#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include "../types.h"
#include "../target-info-armv7.h"

int main( int argc, char* argv[] ) {
	Helix::BuiltinTypes::Init();
	Helix::PhysicalRegisters::Init();

	int result = Catch::Session().run( argc, argv );
	return result;
}
