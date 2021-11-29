#pragma once

#include <stdint.h>
#include <vector>
#include <string>

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

		bool operator==(const Use& other) const;

	private:
		Instruction* m_User;
		uint16_t     m_OperandIndex;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	using Integer = uint64_t;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	template <typename T>
	struct ValueTraits;

	class Function;
	class BasicBlock;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	enum ValueType
	{
		kValue_VirtualRegisterName,
		kValue_ConstantInt,
		kValue_ConstantFloat,
		kValue_BasicBlock,
		kValue_Function,
		kValue_Void,
		kValue_Undefined
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class Value
	{
	private:
		using UseList = std::vector<Use>;

	public:
		using use_iterator       = UseList::iterator;
		using const_use_iterator = UseList::const_iterator;

		Value(ValueType type, const Type* ty)
		    : m_ValueID(type), m_Type(ty)
		{ }

		template <typename T>
		inline bool IsA() const
		{
			return m_ValueID == ValueTraits<T>::ID;
		}

		void AddUse(Instruction* user, uint16_t operandIndex);
		void RemoveUse(Instruction* user, uint16_t operandIndex);

		inline const Type* GetType()            const { return m_Type;         }
		inline size_t      GetCountUses()       const { return m_Users.size(); }
		inline const Use   GetUse(size_t index) const { return m_Users[index]; }

		use_iterator uses_begin() { return m_Users.begin(); }
		use_iterator uses_end()   { return m_Users.end();   }

		const_use_iterator uses_begin() const { return m_Users.begin(); }
		const_use_iterator uses_end()   const { return m_Users.begin(); }

		void SetType(const Type* ty) { m_Type = ty; }

	private:
		ValueType   m_ValueID = kValue_Undefined;
		const Type* m_Type    = nullptr;
		UseList     m_Users;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class VoidValue : public Value
	{
	public:
		VoidValue(): Value(kValue_Void, nullptr) { }

		static VoidValue* Get()
		{
			static VoidValue v;
			return &v;
		}
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	template <typename T>
	inline T* value_cast(Value* v)
	{
		if (!v->IsA<T>())
			return nullptr;

		return static_cast<T*>(v);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	template <typename T>
	inline const T* value_cast(const Value* v)
	{
		if (!v->IsA<T>())
			return nullptr;

		return static_cast<const T*>(v);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

	class FunctionDef : public Value
	{
		FunctionDef(const std::string& name)
		    : Value(kValue_Function, BuiltinTypes::GetFunctionType()),
		      m_Name(name)
		{ }

	public:
		static FunctionDef* Create(const std::string& name)
		{
			return new FunctionDef(name);
		}

		const char* GetName() const { return m_Name.c_str(); }

	private:
		std::string m_Name;
	};

	IMPLEMENT_VALUE_TRAITS( FunctionDef, kValue_Function );

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class VirtualRegisterName : public Value
	{
	public:
		VirtualRegisterName(const Type* ty)
		    : Value(kValue_VirtualRegisterName, ty)
		{ }

		static VirtualRegisterName* Create(const Type* type, const char* name);
		static VirtualRegisterName* Create(const Type* type);

		inline const char* GetDebugName() const { return m_DebugName; }

	private:
		const char* m_DebugName = nullptr;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class ConstantInt : public Value
	{
	public:
		ConstantInt(const Type* ty)
		    : Value(kValue_ConstantInt, ty)
		{ }

		static ConstantInt* Create(const Type* ty, Integer value);

		inline Integer GetIntegralValue() const { return m_Integer; }

		bool CanFitInType(const IntegerType* ty) const;

	private:
		Integer     m_Integer = 0;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class ConstantFloat : public Value
	{
	public:
		ConstantFloat(const Type* ty)
		    : Value(kValue_ConstantFloat, ty)
		{ }
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	IMPLEMENT_VALUE_TRAITS( VirtualRegisterName,  kValue_VirtualRegisterName );
	IMPLEMENT_VALUE_TRAITS( ConstantInt,          kValue_ConstantInt         );
	IMPLEMENT_VALUE_TRAITS( ConstantFloat,        kValue_ConstantFloat       );
	IMPLEMENT_VALUE_TRAITS( BlockBranchTarget,    kValue_BasicBlock          );
	IMPLEMENT_VALUE_TRAITS( VoidValue,            kValue_Void                );

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
