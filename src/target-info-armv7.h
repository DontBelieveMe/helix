#pragma once

#include "target-info.h"

namespace Helix
{
	class PhysicalRegisterName;
	class Type;
	namespace ARMv7
	{
		const Type* PointerType();
		size_t TypeSize(const Type* ty);
	}

	namespace PhysicalRegisters
	{
		enum ArmV7RegisterID
		{
			R0,
			R1,
			R2,
			R3,
			R4,
			R5,
			R6,
			R7,
			R8,
			R9,
			R10,
			R11,
			R12,
			R13,
			R14,
			R15,
			NumRegisters,

			PC = R15,
			LR = R14,
			SP = R13,
			IP = R12,
			FP = R11
		};

		void Init();

		PhysicalRegisterName* GetRegister(const Type* type, ArmV7RegisterID id);
		const char* GetRegisterString(ArmV7RegisterID id);
	}

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
