#include "catch.hpp"
#include "..\value.h"
#include "..\instructions.h"
#include "..\types.h"

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

