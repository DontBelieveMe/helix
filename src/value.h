#pragma once

#include <stdint.h>

#include "types.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define IMPLEMENT_VALUE_TRAITS(ClassName, ValueID) \
	template <> \
	struct ValueTraits<ClassName> { \
		static constexpr Helix::ValueType ID = ValueID; \
	};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Helix
{
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	using Integer = uint64_t;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	template <typename T>
	struct ValueTraits;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	enum ValueType
	{
		kValue_VirtualRegisterName,
		kValue_ConstantInt,
		kValue_ConstantFloat,
		kValue_Undefined
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class Value
	{
	public:
		Value(ValueType type, const Type* ty): m_ValueID(type), m_Type(ty) { }

		template <typename T>
		bool IsA() const
		{
			return m_ValueID == ValueTraits<T>::ID;
		}

		inline const Type* GetType() const { return m_Type; }

	private:
		ValueType m_ValueID = kValue_Undefined;
		const Type*  m_Type = nullptr;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	template <typename T>
	inline T* value_cast(Value* v)
	{
		if (!v->IsA<T>())
			return nullptr;

		return static_cast<T*>(v);
	}

	template <typename T>
	inline const T* value_cast(const Value* v)
	{
		if (!v->IsA<T>())
			return nullptr;

		return static_cast<const T*>(v);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class VirtualRegisterName : public Value
	{
	public:
		VirtualRegisterName(const Type* ty): Value(kValue_VirtualRegisterName, ty) { }

		static VirtualRegisterName* Create(const Type* type, const char* name);
		static VirtualRegisterName* Create(const Type* type);

		const char* GetDebugName() const { return m_DebugName; }
		size_t GetSlot() const { return m_Slot; }

	private:
		const char* m_DebugName = nullptr;
		size_t      m_Slot      = 0;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class ConstantInt : public Value
	{
	public:
		ConstantInt(const Type* ty): Value(kValue_ConstantInt, ty) { }

		static ConstantInt* Create(const Type* ty, Integer value);

		inline Integer GetIntegralValue() const { return m_Integer; }

	private:
		Integer     m_Integer = 0;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class ConstantFloat : public Value
	{
	public:
		ConstantFloat(const Type* ty): Value(kValue_ConstantFloat, ty) { }
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	IMPLEMENT_VALUE_TRAITS(VirtualRegisterName, kValue_VirtualRegisterName);
	IMPLEMENT_VALUE_TRAITS(ConstantInt, kValue_ConstantInt);
	IMPLEMENT_VALUE_TRAITS(ConstantFloat, kValue_ConstantFloat);

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

}
