#pragma once

namespace Helix
{
	class TargetInfo
	{
	public:
		virtual ~TargetInfo() = default;

		enum IntType
		{
			kIntType_SignedChar,
			kIntType_UnsignedChar,
			kIntType_SignedShort,
			kIntType_UnsignedShort,
			kIntType_SignedInt,
			kIntType_UnsignedInt,
			kIntType_SignedLong,
			kIntType_UnsignedLong,
			kIntType_SignedLongLong,
			kIntType_UnsignedLongLong
		};

		virtual IntType GetSizeType() const = 0;
		virtual size_t  GetIntByteWidth(IntType ty) const = 0;

		size_t GetIntBitWidth(IntType ty) const { return GetIntByteWidth(ty) * 8; }

	private:
	};
}
