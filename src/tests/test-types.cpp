/**
 * @file test-types.cpp
 * @author Barney Wilks
 */

 /* Helix Core Includes */
#include "..\types.h"

/* Testing Library Includes */
#include "catch.hpp"

using namespace Helix;

/*********************************************************************************************************************/

TEST_CASE("type_cast returns NULL given a NULL input", "[Types]")
{
	const IntegerType* result = Helix::type_cast<IntegerType>(nullptr);

	REQUIRE(result == nullptr);
}

/*********************************************************************************************************************/

TEST_CASE("type_cast returns NULL where the given pointer is not of the given type", "[Types]")
{
	const FunctionType* f = FunctionType::Create(BuiltinTypes::GetVoidType(), {});

	const IntegerType* result = Helix::type_cast<IntegerType>(f);

	REQUIRE(result == nullptr);
}

/*********************************************************************************************************************/

TEST_CASE("type_cast returns valid pointer where the given input pointer is of the right type", "[Types]")
{
	const FunctionType* f = FunctionType::Create(BuiltinTypes::GetVoidType(), {});
	const FunctionType* a = Helix::type_cast<FunctionType>(f);

	REQUIRE(a == f);
}

/*********************************************************************************************************************/
