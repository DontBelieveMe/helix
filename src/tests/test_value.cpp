#include "catch.hpp"
#include "..\value.h"
#include "..\instructions.h"

using namespace Helix;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("Value single user", "[Value]")
{
	VirtualRegisterName* vreg = VirtualRegisterName::Create(BuiltinTypes::GetInt32());
	RetInsn* ret = Helix::CreateRet(vreg);

	REQUIRE(vreg->GetCountUses() == 1);
	REQUIRE(vreg->GetUse(0) == Use(ret, 0));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("Value multiple users", "[Value]")
{
	VirtualRegisterName* vreg = VirtualRegisterName::Create(BuiltinTypes::GetInt32());
	RetInsn* ret = Helix::CreateRet(vreg);
	RetInsn* ret2 = Helix::CreateRet(vreg);

	REQUIRE(vreg->GetCountUses() == 2);
	REQUIRE(vreg->GetUse(0) == Use(ret, 0));
	REQUIRE(vreg->GetUse(1) == Use(ret2, 0));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("Value removed user", "[Value]")
{
	VirtualRegisterName* vreg = VirtualRegisterName::Create(BuiltinTypes::GetInt32());
	ConstantInt* ci = ConstantInt::Create(BuiltinTypes::GetInt32(), 1234);

	RetInsn* ret = Helix::CreateRet(vreg);
	RetInsn* ret2 = Helix::CreateRet(vreg);

	ret2->SetOperand(0, ci);

	REQUIRE(vreg->GetCountUses() == 1);
	REQUIRE(vreg->GetUse(0) == Use(ret, 0));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("Value all users removed", "[Value]")
{
	VirtualRegisterName* vreg = VirtualRegisterName::Create(BuiltinTypes::GetInt32());
	ConstantInt* ci = ConstantInt::Create(BuiltinTypes::GetInt32(), 1234);

	RetInsn* ret = Helix::CreateRet(vreg);
	RetInsn* ret2 = Helix::CreateRet(vreg);

	ret2->SetOperand(0, ci);
	ret->SetOperand(0, ci);

	REQUIRE(vreg->GetCountUses() == 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("New value has no users", "[Value]")
{
	VirtualRegisterName* vreg = VirtualRegisterName::Create(BuiltinTypes::GetInt32());
	REQUIRE(vreg->GetCountUses() == 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("ConstantInt::Create (Normal)", "[Types]")
{
	ConstantInt* ci = ConstantInt::Create(BuiltinTypes::GetInt32(), 1234);

	REQUIRE(ci);
	REQUIRE(ci->GetType() == BuiltinTypes::GetInt32());
	REQUIRE(ci->GetIntegralValue() == 1234);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

