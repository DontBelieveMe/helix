#include "target-info-armv7.h"
#include "system.h"

using namespace Helix;

TargetInfo::IntType TargetInfo_ArmV7::GetSizeType() const
{
    return kIntType_UnsignedLong;
}

size_t TargetInfo_ArmV7::GetIntByteWidth(IntType ty) const
{
	switch (ty) {
	case kIntType_UnsignedChar:
	case kIntType_SignedChar:
		return 1;

	case kIntType_UnsignedShort:
	case kIntType_SignedShort:
		return 2;

	case kIntType_UnsignedInt:
	case kIntType_SignedInt:
		return 4;

	case kIntType_UnsignedLong:
	case kIntType_SignedLong:
		return 4;

	case kIntType_UnsignedLongLong:
	case kIntType_SignedLongLong:
		return 8;

    default:
        helix_unreachable("unknown int type");
        break;
	}

    return 0;
}