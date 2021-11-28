#pragma once

namespace Helix
{
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	enum TypeID
	{
		/// 32 bit IEEE 754 floating point number (float in C terms)
		kType_Float32,

		/// 64 bit IEE 754 floating point number (double in C terms)
		kType_Float64,

		/// 8 bit twos complement integer 
		kType_Int8,

		/// 16 bit twos complement integer 
		kType_Int16,

		/// 32 bit twos complement integer 
		kType_Int32,

		/// 64 bit twos complement integer 
		kType_Int64,

		/// A basic block branch target, e.g the target of a branch within a
		/// function
		kType_LabelType,

		/// A function branch target, e.g. a call
		kType_FunctionType,

		/// Opaque pointer type, represents a memory address.
		kType_Pointer,

		/// Unknown type, a "invalid" state for Type.
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
