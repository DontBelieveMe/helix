#pragma once

#include <vector>
#include <string>

#define IMPLEMENT_TYPE_TRAITS(ClassName, BaseTypeID) \
	template <> \
	struct TypeTraits<ClassName> { \
		static constexpr Helix::TypeID ID = BaseTypeID; \
	};

namespace Helix
{
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	enum TypeID
	{
		/// 32 bit IEEE 754 floating point number (float in C terms)
		kType_Float32,

		/// 64 bit IEE 754 floating point number (double in C terms)
		kType_Float64,

		// An arbitrary bit width twos complement integer
		kType_Integer,

		/// A basic block branch target, e.g the target of a branch within a
		/// function
		kType_LabelType,

		/// A function branch target, e.g. a call
		kType_FunctionType,

		/// Opaque pointer type, represents a memory address.
		kType_Pointer,

		/// Void (aka no) return type
		kType_Void,

		/// Unknown type, a "invalid" state for Type.
		kType_Undefined,

		/// User defined structure
		kType_Struct
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	template <typename T>
	struct TypeTraits;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class Type
	{
	public:
		virtual ~Type() = default;

		Type(TypeID baseType)
			: m_BaseID(baseType) { }

		template <typename T>
		bool IsA() const
		{
			return m_BaseID == TypeTraits<T>::ID;
		}

		static const Type* Create(TypeID base);

		TypeID GetTypeID() const { return m_BaseID; }

		bool IsPointer() const { return m_BaseID == kType_Pointer; }
		bool IsIntegral() const { return m_BaseID == kType_Integer; }

	private:
		TypeID m_BaseID = kType_Undefined;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class IntegerType : public Type
	{
	public:
		IntegerType(size_t width)
			: Type(kType_Integer), m_BitWidth(width)
		{ }

		static const IntegerType* Create(size_t width);

		size_t GetBitWidth() const { return m_BitWidth; }

	private:
		size_t m_BitWidth;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	class StructType : public Type
	{
	public:
		using FieldList = std::vector<const Type*>;

		using fields_iterator = FieldList::iterator;
		using const_fields_iterator = FieldList::const_iterator;

	public:
		StructType(const std::string& name, const FieldList& fields)
			: Type(kType_Struct), m_Name(name), m_Members(fields)
		{ }

		static const StructType* CreateNamedStruct(const std::string& name, const FieldList& fields)
		{
			return new StructType(name, fields);
		}

		static const StructType* CreateUnnamedStruct(const FieldList& fields)
		{
			static size_t anonStructCounter = 0;

			const std::string name = "anon." + std::to_string(anonStructCounter);
			anonStructCounter += 1;

			return CreateNamedStruct(name, fields);
		}

		const char* GetName() const { return m_Name.c_str(); }

		size_t GetCountFields() const { return m_Members.size(); }

		fields_iterator fields_begin() { return m_Members.begin(); }
		fields_iterator fields_end() { return m_Members.end(); }

		const_fields_iterator fields_begin() const { return m_Members.begin(); }
		const_fields_iterator fields_end() const { return m_Members.end(); }

	private:
		std::vector<const Type*> m_Members;
		std::string m_Name;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	IMPLEMENT_TYPE_TRAITS(IntegerType, kType_Integer);
	IMPLEMENT_TYPE_TRAITS(StructType,  kType_Struct);

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	template <typename T>
	inline const T* type_cast(const Type* type)
	{
		if (!type)
			return nullptr;

		if (type->IsA<T>()) {
			return static_cast<const T*>(type);
		}

		return nullptr;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	namespace BuiltinTypes
	{
		void Init();
		void Destroy();

		const Type* GetInt32();
		const Type* GetInt64();
		const Type* GetInt16();
		const Type* GetInt8();

		const Type* GetFloat32();
		const Type* GetFloat64();

		const Type* GetLabelType();
		const Type* GetFunctionType();
		const Type* GetPointer();

		const Type* GetVoidType();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
