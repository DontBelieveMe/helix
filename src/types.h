#pragma once

namespace Helix
{
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	enum TypeID
	{
		kType_Float32,
		kType_Float64,

		kType_Int8,
		kType_Int16,
		kType_Int32,
		kType_Int64,

		kType_LabelType,
		kType_FunctionType,
		kType_Pointer,

		kType_Undefined
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class Type
	{
	public:
		Type(TypeID baseType)
			: m_BaseID(baseType) { }

		static const Type* Create(TypeID base);

		TypeID GetTypeID() const { return m_BaseID; }

		bool IsPointer() const { return m_BaseID == kType_Pointer; }

	private:
		TypeID m_BaseID = kType_Undefined;
	};


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
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
