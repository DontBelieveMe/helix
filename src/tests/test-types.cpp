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

TEST_CASE("Creating a new ArrayType", "[Types]")
{
	const ArrayType* type = ArrayType::Create(10, BuiltinTypes::GetInt32());

	REQUIRE(type->GetCountElements() == 10);
	REQUIRE(type->GetBaseType() == BuiltinTypes::GetInt32());
}

/*********************************************************************************************************************/

TEST_CASE("Creating a new named StructType", "[Types]")
{
	StructType::FieldList fields { BuiltinTypes::GetInt32() };
	const StructType* type = StructType::Create("hello", fields);

	REQUIRE(strcmp(type->GetName(), "hello") == 0);
	REQUIRE(type->GetCountFields() == 1);
	REQUIRE(type->GetField(0) == BuiltinTypes::GetInt32());
}

/*********************************************************************************************************************/

TEST_CASE("Creating a new unnamed StructType", "[Types]")
{
	StructType::FieldList fields { BuiltinTypes::GetInt32() };
	const StructType* type = StructType::Create(fields);

	REQUIRE(type->GetCountFields() == 1);
	REQUIRE(type->GetField(0) == BuiltinTypes::GetInt32());
}

/*********************************************************************************************************************/

TEST_CASE("Creating two different unnamed StructTypes with different names", "[Types]")
{
	StructType::FieldList fields { BuiltinTypes::GetInt32() };

	const StructType* a = StructType::Create(fields);
	const StructType* b = StructType::Create(fields);

	REQUIRE(strcmp(a->GetName(), b->GetName()) != 0);
}

/*********************************************************************************************************************/

TEST_CASE("Creating a new FunctionType", "[Types]")
{
	const FunctionType::ParametersList params { BuiltinTypes::GetInt32() };

	const FunctionType* type = FunctionType::Create(BuiltinTypes::GetVoidType(), params);

	REQUIRE(type->GetReturnType() == BuiltinTypes::GetVoidType());
	REQUIRE(type->GetParameters() == params);
}

/*********************************************************************************************************************/

TEST_CASE("FunctionType::CopyWithDifferentReturnType", "[Types]")
{
	const FunctionType::ParametersList params { BuiltinTypes::GetInt32() };
	const FunctionType* old = FunctionType::Create(BuiltinTypes::GetVoidType(), params);

	const FunctionType* type = old->CopyWithDifferentReturnType(BuiltinTypes::GetInt32());

	REQUIRE(type->GetParameters() == params);
	REQUIRE(type->GetReturnType() == BuiltinTypes::GetInt32());
}

/*********************************************************************************************************************/
