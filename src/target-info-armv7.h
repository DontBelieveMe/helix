#pragma once

#include "target-info.h"

namespace Helix
{
	class TargetInfo_ArmV7 : public TargetInfo
	{
	public:
		virtual IntType GetSizeType() const override;
		virtual size_t GetIntByteWidth(IntType ty) const override;

		virtual size_t GetCharByteWidth() const override;
		virtual size_t GetShortByteWidth() const override;
		virtual size_t GetIntByteWidth() const override;
		virtual size_t GetLongByteWidth() const override;
		virtual size_t GetLongLongByteWidth() const override;

		virtual size_t GetPointerByteWidth() const override;
	};
}
