#pragma once

namespace Helix
{
	enum TypeID
	{
		kType_Float32,
		kType_Float64,

		kType_Int8,
		kType_Int16,
		kType_Int32,
		kType_Int64,

		kType_Undefined
	};

	class Type
	{
	public:
		Type(TypeID baseType)
			: m_BaseID(baseType) { }

		static const Type* Create(TypeID base);

	private:
		TypeID m_BaseID = kType_Undefined;
	};

	namespace BuiltinTypes
	{
		void Init();

		const Type* GetInt32();
		const Type* GetInt64();
		const Type* GetInt16();
		const Type* GetInt8();

		const Type* GetFloat32();
		const Type* GetFloat64();
	}
}
