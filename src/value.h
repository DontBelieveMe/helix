#pragma once

#include <stdint.h>

#include "types.h"

#define IMPLEMENT_VALUE_TRAITS(ClassName, ValueID) \
	template <> \
	struct ValueTraits<ClassName> { \
		static constexpr Helix::ValueType ID = ValueID; \
	};

namespace Helix
{
	using Integer = uint64_t;

	template <typename T>
	struct ValueTraits;

	enum ValueType
	{
		kValue_VirtualRegisterName,
		kValue_ConstantInt,
		kValue_ConstantFloat,
		kValue_Undefined
	};

	class Value
	{
	public:
		Value(ValueType type): m_Type(type) { }

		template <typename T>
		bool IsA()
		{
			return m_Type == ValueTraits<T>::ID;
		}

	private:
		ValueType m_Type = kValue_Undefined;
	};

	template <typename T>
	inline T* value_cast(Value* v)
	{
		if (!v->IsA<T>())
			return nullptr;

		return static_cast<T*>(v);
	}

	class VirtualRegisterName : public Value
	{
	public:
		VirtualRegisterName(): Value(kValue_VirtualRegisterName) { }

		static VirtualRegisterName* Create(const char* name);
		static VirtualRegisterName* Create();

		const char* GetDebugName() const { return m_DebugName; }

	private:
		const char* m_DebugName = nullptr;
	};

	class ConstantInt : public Value
	{
	public:
		ConstantInt(): Value(kValue_ConstantInt) { }

		static ConstantInt* Create(const Type* ty, Integer value);

	private:
		Integer     m_Integer = 0;
		const Type* m_Type    = nullptr;
	};

	class ConstantFloat : public Value
	{
	public:
		ConstantFloat(): Value(kValue_ConstantFloat) { }		
	};

	IMPLEMENT_VALUE_TRAITS(VirtualRegisterName, kValue_VirtualRegisterName);
	IMPLEMENT_VALUE_TRAITS(ConstantInt, kValue_ConstantInt);
	IMPLEMENT_VALUE_TRAITS(ConstantFloat, kValue_ConstantFloat);
}
