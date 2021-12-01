#include "target-info-armv7.h"
#include "system.h"

using namespace Helix;

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