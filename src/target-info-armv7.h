#pragma once

#include "target-info.h"

namespace Helix
{
	class TargetInfo_ArmV7 : public TargetInfo
	{
	public:
		virtual IntType GetSizeType() const override
		{
			return kIntType_UnsignedLong;
		}

		virtual size_t GetIntWidth(IntType ty) const override
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
			}
		}
	};
}
