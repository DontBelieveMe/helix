#include "catch.hpp"
#include "..\print.h"

using namespace Helix;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("TextOutputStream to string, no overflow", "[Print]")
{
	char buf[256] = {};
	TextOutputStream str(buf, sizeof(buf));
	str.Write("Hello, ");
	str.Write("World!");

	REQUIRE(strcmp(buf, "Hello, World!") == 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("TextOutputStream to string, overflow", "[Print]")
{
	char buf[10] = {};
	TextOutputStream str(buf, sizeof(buf));

	str.Write("HelloThisIsATest");

	REQUIRE(strcmp(buf, "HelloThis") == 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

