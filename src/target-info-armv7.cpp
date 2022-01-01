#include "target-info-armv7.h"
#include "system.h"
#include "value.h"

#include <array>
#include <numeric>

using namespace Helix;

struct RegisterData
{
	const char* str;
	PhysicalRegisterName byte_reg;
	PhysicalRegisterName halfword_reg;
	PhysicalRegisterName word_reg;
};

static std::array<RegisterData, PhysicalRegisters::NumRegisters> s_Registers;

void PhysicalRegisters::Init()
{
	#define PHYSICAL_REGISTER(index, string_name, enum_name) \
		s_Registers[index] = \
		{ \
			string_name, \
			PhysicalRegisterName(BuiltinTypes::GetInt8(), PhysicalRegisters::enum_name), \
			PhysicalRegisterName(BuiltinTypes::GetInt16(), PhysicalRegisters::enum_name), \
			PhysicalRegisterName(BuiltinTypes::GetInt32(), PhysicalRegisters::enum_name), \
		};


	PHYSICAL_REGISTER(0,  "r0",  R0);
	PHYSICAL_REGISTER(1,  "r1",  R1);
	PHYSICAL_REGISTER(2,  "r2",  R2);
	PHYSICAL_REGISTER(3,  "r3",  R3);
	PHYSICAL_REGISTER(4,  "r4",  R4);
	PHYSICAL_REGISTER(5,  "r5",  R5);
	PHYSICAL_REGISTER(6,  "r6",  R6);
	PHYSICAL_REGISTER(7,  "r7",  R7);
	PHYSICAL_REGISTER(8,  "r8",  R8);
	PHYSICAL_REGISTER(9,  "r9",  R9);
	PHYSICAL_REGISTER(10, "r10", R10);
	PHYSICAL_REGISTER(11, "r11", R11);
	PHYSICAL_REGISTER(12, "r12", R12);
	PHYSICAL_REGISTER(13, "r13", R13);
	PHYSICAL_REGISTER(14, "r14", R14);
	PHYSICAL_REGISTER(15, "r15", R15);
}

PhysicalRegisterName* PhysicalRegisters::GetRegister(const Type* type, ArmV7RegisterID id)
{
	if (const IntegerType* int_type = type_cast<IntegerType>(type)) {
		RegisterData& reg_data = s_Registers[id];
		switch (int_type->GetBitWidth()) {
		case 8: return &reg_data.byte_reg;
		case 16: return &reg_data.halfword_reg;
		case 32: return &reg_data.word_reg;
		default:
			helix_unreachable("physical register sizes can only be 8/16/32 bits");
			break;
		}

		return nullptr;
	}

	helix_unreachable("physical registers can only have integral type");

	return nullptr;
}

const char* PhysicalRegisters::GetRegisterString(ArmV7RegisterID id)
{
	return s_Registers[id].str;
}

const Type* ARMv7::PointerType()
{
	return BuiltinTypes::GetInt32();
}

size_t ARMv7::TypeSize(const Type* ty)
{
	switch (ty->GetTypeID()) {
	case kType_Integer:
		return type_cast<IntegerType>(ty)->GetBitWidth() / 8;
	case kType_Array: {
		const ArrayType* arr = type_cast<ArrayType>(ty);
		return arr->GetCountElements() * ARMv7::TypeSize(arr->GetBaseType());
	}
	case kType_Pointer: {
		return 4;
	}
	case kType_Struct: {
		const StructType* st = type_cast<StructType>(ty);
		return std::accumulate(
			st->fields_begin(),
			st->fields_end(),
			size_t(0),
			[](size_t v, const Type* field) -> size_t {
				return v + ARMv7::TypeSize(field);
			}
		);
	}
	default:
		helix_unimplemented("TypeSize not implemented for type category");
		break;
	}

	return 0;
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