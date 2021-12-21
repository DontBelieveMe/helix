#include "catch.hpp"

#include "..\value.h"
#include "..\types.h"

using namespace Helix;

/*********************************************************************************************************************/

TEST_CASE("ConstantInt::Create (Normal)", "[Types]")
{
	ConstantInt* ci = ConstantInt::Create(BuiltinTypes::GetInt32(), 1234);

	REQUIRE(ci);
	REQUIRE(ci->GetType() == BuiltinTypes::GetInt32());
	REQUIRE(ci->GetIntegralValue() == 1234);
}

/*********************************************************************************************************************/

TEST_CASE("ConstantInt::Create (Same Inputs, Cached)", "[Types]")
{
	ConstantInt* ci1 = ConstantInt::Create(BuiltinTypes::GetInt32(), 1234);
	ConstantInt* ci2 = ConstantInt::Create(BuiltinTypes::GetInt32(), 1234);

	REQUIRE(ci1);
	REQUIRE(ci2);
	REQUIRE(ci1 == ci2);

	REQUIRE(ci1->GetIntegralValue() == 1234);
	REQUIRE(ci1->GetType() == BuiltinTypes::GetInt32());
}

/*********************************************************************************************************************/

TEST_CASE("ConstantInt::Create (Different Types/Same Value, Not Cached)", "[Types]")
{
	ConstantInt* ci1 = ConstantInt::Create(BuiltinTypes::GetInt32(), 1234);
	ConstantInt* ci2 = ConstantInt::Create(BuiltinTypes::GetInt16(), 1234);

	REQUIRE(ci1 != ci2);

	REQUIRE(ci1->GetIntegralValue() == 1234);
	REQUIRE(ci1->GetType() == BuiltinTypes::GetInt32());

	REQUIRE(ci2->GetIntegralValue() == 1234);
	REQUIRE(ci2->GetType() == BuiltinTypes::GetInt16());
}

/*********************************************************************************************************************/

TEST_CASE("ConstantInt::Create (Same Types/Different Value, Not Cached)", "[Types]")
{
	ConstantInt* ci1 = ConstantInt::Create(BuiltinTypes::GetInt32(), 1234);
	ConstantInt* ci2 = ConstantInt::Create(BuiltinTypes::GetInt32(), 652);

	REQUIRE(ci1 != ci2);

	REQUIRE(ci1->GetIntegralValue() == 1234);
	REQUIRE(ci1->GetType() == BuiltinTypes::GetInt32());

	REQUIRE(ci2->GetIntegralValue() == 652);
	REQUIRE(ci2->GetType() == BuiltinTypes::GetInt32());
}

/*********************************************************************************************************************/

TEST_CASE("ConstantInt::Create (Different Types/Different Value, Not Cached)", "[Types]")
{
	ConstantInt* ci1 = ConstantInt::Create(BuiltinTypes::GetInt32(), 1234);
	ConstantInt* ci2 = ConstantInt::Create(BuiltinTypes::GetInt64(), 644);

	REQUIRE(ci1 != ci2);

	REQUIRE(ci1->GetIntegralValue() == 1234);
	REQUIRE(ci1->GetType() == BuiltinTypes::GetInt32());

	REQUIRE(ci2->GetIntegralValue() == 644);
	REQUIRE(ci2->GetType() == BuiltinTypes::GetInt64());
}

/*********************************************************************************************************************/

TEST_CASE("ConstantInt::CanFitInType where the value can always", "ConstantInt")
{
	ConstantInt* i32 = ConstantInt::Create(BuiltinTypes::GetInt32(), 10);

	REQUIRE(i32->CanFitInType(type_cast<IntegerType>(BuiltinTypes::GetInt8())));
	REQUIRE(i32->CanFitInType(type_cast<IntegerType>(BuiltinTypes::GetInt16())));
	REQUIRE(i32->CanFitInType(type_cast<IntegerType>(BuiltinTypes::GetInt32())));
	REQUIRE(i32->CanFitInType(type_cast<IntegerType>(BuiltinTypes::GetInt64())));
}

/*********************************************************************************************************************/

TEST_CASE("ConstantInt::CanFitInType where the value can't always", "ConstantInt")
{
	ConstantInt* i32 = ConstantInt::Create(BuiltinTypes::GetInt32(), 65536);

	REQUIRE(i32->CanFitInType(type_cast<IntegerType>(BuiltinTypes::GetInt8())) == false);
	REQUIRE(i32->CanFitInType(type_cast<IntegerType>(BuiltinTypes::GetInt16())) == false);
	REQUIRE(i32->CanFitInType(type_cast<IntegerType>(BuiltinTypes::GetInt32())) == true);
	REQUIRE(i32->CanFitInType(type_cast<IntegerType>(BuiltinTypes::GetInt64())) == true);
}

/*********************************************************************************************************************/
