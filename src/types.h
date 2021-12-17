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

		/// An array of elements
		kType_Array,

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

		Type(TypeID baseType);

		template <typename T>
		bool IsA() const
		{
			return m_BaseID == TypeTraits<T>::ID;
		}

		TypeID GetTypeID()   const { return m_BaseID;                  }
		bool   IsPointer()   const { return m_BaseID == kType_Pointer; }
		bool   IsIntegral()  const { return m_BaseID == kType_Integer; }
		bool   IsStruct()    const { return m_BaseID == kType_Struct;  }
		bool   IsArray()    const { return m_BaseID == kType_Array; }

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

	class ArrayType : public Type
	{
	public:
		ArrayType(size_t nElements, const Type* baseType);

		static const ArrayType* Create(size_t nElements, const Type* baseType);

		size_t GetCountElements() const { return m_CountElements; }
		const Type* GetBaseType() const { return m_BaseType; }

	private:
		size_t      m_CountElements;
		const Type* m_BaseType;
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

		static const StructType* Create(const std::string& name, const FieldList& fields);
		static const StructType* Create(const FieldList& fields);

		const char* GetName()        const { return m_Name.c_str();   }
		size_t      GetCountFields() const { return m_Members.size(); }

		fields_iterator       fields_begin()       { return m_Members.begin(); }
		fields_iterator       fields_end()         { return m_Members.end();   }
		const_fields_iterator fields_begin() const { return m_Members.begin(); }
		const_fields_iterator fields_end()   const { return m_Members.end();   }

		const Type* GetField(size_t index) const { return m_Members[index]; }

	private:
		std::vector<const Type*> m_Members;
		std::string              m_Name;
	};

	class FunctionType : public Type
	{
	public:
		using ParametersList = std::vector<const Type*>;

		FunctionType(const Type* returnType, const ParametersList& params);

		static const FunctionType* Create(const Type* returnType, const ParametersList& params);

		const Type*           GetReturnType() const { return m_ReturnType; }
		const ParametersList& GetParameters() const { return m_Parameters; }

		ParametersList::const_iterator params_begin() const { return m_Parameters.begin(); }
		ParametersList::const_iterator params_end() const { return m_Parameters.end(); }

		/**
		 * Create a new FunctionType with the same parameters as this one, but just
		 * with a new (and hopefully different return type).
		 * 
		 * @param newReturnType The new return type that the FunctionType returned should have.
		 * @return const FunctionType* A new distint FunctionType, with the given return type & same parameters
		 *                             as this.
		 */
		const FunctionType* CopyWithDifferentReturnType(const Type* newReturnType) const;

	private:
		const Type*    m_ReturnType;
		ParametersList m_Parameters;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	IMPLEMENT_TYPE_TRAITS(IntegerType,  kType_Integer);
	IMPLEMENT_TYPE_TRAITS(StructType,   kType_Struct);
	IMPLEMENT_TYPE_TRAITS(ArrayType,    kType_Array);
	IMPLEMENT_TYPE_TRAITS(FunctionType, kType_FunctionType);

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
