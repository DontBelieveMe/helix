#include "target-info-armv7.h"
#include "system.h"
#include "value.h"

#include <array>

using namespace Helix;

struct RegisterData
{
	const char* str;
	PhysicalRegisterName reg_name;
};

static std::array<RegisterData, PhysicalRegisters::NumRegisters> s_Registers;

void PhysicalRegisters::Init()
{
	s_Registers[0] = { "r0", PhysicalRegisterName(BuiltinTypes::GetInt32(), PhysicalRegisters::R0)  };
	s_Registers[1] = { "r1",  PhysicalRegisterName(BuiltinTypes::GetInt32(), PhysicalRegisters::R1) };
	s_Registers[2] = { "r2",  PhysicalRegisterName(BuiltinTypes::GetInt32(), PhysicalRegisters::R2) };
	s_Registers[3] = { "r3",  PhysicalRegisterName(BuiltinTypes::GetInt32(), PhysicalRegisters::R3) };
	s_Registers[4] = { "r4",  PhysicalRegisterName(BuiltinTypes::GetInt32(), PhysicalRegisters::R4) };
	s_Registers[5] = { "r5",  PhysicalRegisterName(BuiltinTypes::GetInt32(), PhysicalRegisters::R5) };
	s_Registers[6] = { "r6",  PhysicalRegisterName(BuiltinTypes::GetInt32(), PhysicalRegisters::R6) };
	s_Registers[7] = { "r7",  PhysicalRegisterName(BuiltinTypes::GetInt32(), PhysicalRegisters::R7) };
	s_Registers[8] = { "r8",  PhysicalRegisterName(BuiltinTypes::GetInt32(), PhysicalRegisters::R8) };
	s_Registers[9] = { "r9",  PhysicalRegisterName(BuiltinTypes::GetInt32(), PhysicalRegisters::R9) };
	s_Registers[10] = { "r10", PhysicalRegisterName(BuiltinTypes::GetInt32(), PhysicalRegisters::R10) };
	s_Registers[11] = { "fp",  PhysicalRegisterName(BuiltinTypes::GetInt32(), PhysicalRegisters::R11) };
	s_Registers[12] = { "ip",  PhysicalRegisterName(BuiltinTypes::GetInt32(), PhysicalRegisters::R12) };
	s_Registers[13] = { "sp",  PhysicalRegisterName(BuiltinTypes::GetInt32(), PhysicalRegisters::R13) };
	s_Registers[14] = { "lr",  PhysicalRegisterName(BuiltinTypes::GetInt32(), PhysicalRegisters::R14) };
	s_Registers[15] = { "pc",  PhysicalRegisterName(BuiltinTypes::GetInt32(), PhysicalRegisters::R15) };
}

PhysicalRegisterName* PhysicalRegisters::GetRegister(ArmV7RegisterID id)
{
	return &s_Registers[id].reg_name;
}

const char* PhysicalRegisters::GetRegisterString(ArmV7RegisterID id)
{
	return s_Registers[id].str;
}


TargetInfo::IntType TargetInfo_ArmV7::GetSizeType() const
{
    return kIntType_UnsignedLong;
}

size_t TargetInfo_ArmV7::GetCharByteWidth()     const { return 1; }
size_t TargetInfo_ArmV7::GetShortByteWidth()    const { return 2; }
size_t TargetInfo_ArmV7::GetIntByteWidth()      const { return 4; }
size_t TargetInfo_ArmV7::GetLongByteWidth()     const { return 4; }
size_t TargetInfo_ArmV7::GetLongLongByteWidth() const { return 8; }
size_t TargetInfo_ArmV7::GetPointerByteWidth()  const { return 4; }

size_t TargetInfo_ArmV7::GetIntByteWidth(IntType ty) const
{
	switch (ty) {
	case kIntType_UnsignedChar:
	case kIntType_SignedChar:
		return this->GetCharByteWidth();

	case kIntType_UnsignedShort:
	case kIntType_SignedShort:
		return this->GetShortByteWidth();

	case kIntType_UnsignedInt:
	case kIntType_SignedInt:
		return this->GetIntByteWidth();

	case kIntType_UnsignedLong:
	case kIntType_SignedLong:
		return this->GetLongByteWidth();

	case kIntType_UnsignedLongLong:
	case kIntType_SignedLongLong:
		return this->GetLongLongByteWidth();

    default:
        helix_unreachable("unknown int type");
        break;
	}

    return 0;
}