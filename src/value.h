#pragma once

#include <stdint.h>
#include <vector>
#include <algorithm>

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

	class Instruction;

	class Use
	{
	public:
		Use(Instruction* insn, uint16_t index)
			: m_User(insn), m_OperandIndex(index)
		{ }

		bool operator==(const Use& other) const
			{ return m_User == other.m_User && m_OperandIndex == other.m_OperandIndex; }

	private:
		Instruction* m_User;
		uint16_t     m_OperandIndex;
	};

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
		kValue_BasicBlock,
		kValue_Function,
		kValue_Undefined
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class Value
	{
	private:
		using UseList = std::vector<Use>;

	public:
		using use_iterator = UseList::iterator;
		using const_use_iterator = UseList::const_iterator;

		Value(ValueType type, const Type* ty): m_ValueID(type), m_Type(ty) { }

		template <typename T>
		bool IsA() const
		{
			return m_ValueID == ValueTraits<T>::ID;
		}

		inline const Type* GetType() const { return m_Type; }

		void AddUse(Instruction* user, uint16_t operandIndex)
			{ m_Users.push_back({user, operandIndex}); }

		void RemoveUse(Instruction* user, uint16_t operandIndex)
		{
			m_Users.erase(std::remove(m_Users.begin(), m_Users.end(), Use(user, operandIndex)), m_Users.end());
		}

		use_iterator uses_begin() { return m_Users.begin(); }
		use_iterator uses_end() { return m_Users.end(); }

		size_t GetCountUses() const { return m_Users.size(); }

		const Use GetUse(size_t index) const { return m_Users[index]; }

	private:
		ValueType m_ValueID = kValue_Undefined;
		const Type*  m_Type = nullptr;
		UseList m_Users;
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
	
	class Function;
	class BasicBlock;

	class FunctionBranchTarget : public Value
	{
	public:
		FunctionBranchTarget(Function* parent)
			: Value(kValue_Function, BuiltinTypes::GetFunctionType()),
		      m_Parent(parent)
		{ }

		Function* GetParent() const { return m_Parent; }

	private:
		Function* m_Parent;
	};

	class BlockBranchTarget : public Value
	{
	public:
		BlockBranchTarget(BasicBlock* parent)
			: Value(kValue_BasicBlock, BuiltinTypes::GetLabelType()),
		      m_Parent(parent)
		{ }

		BasicBlock* GetParent() const { return m_Parent; }

	private:
		BasicBlock* m_Parent;
	};

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

	IMPLEMENT_VALUE_TRAITS(FunctionBranchTarget, kValue_Function);
	IMPLEMENT_VALUE_TRAITS(BlockBranchTarget, kValue_BasicBlock);

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
