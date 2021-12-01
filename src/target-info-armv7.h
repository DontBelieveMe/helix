#pragma once

#include "target-info.h"

namespace Helix
{
	class TargetInfo_ArmV7 : public TargetInfo
	{
	public:
		virtual IntType GetSizeType() const override;
		virtual size_t GetIntByteWidth(IntType ty) const override;

	};
}
