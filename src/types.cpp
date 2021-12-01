#include "types.h"

using namespace Helix;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define BUILTIN_TYPE(name) \
	static const Type* s_##name; \
	const Type* BuiltinTypes::Get##name() { return s_##name; }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BUILTIN_TYPE(Int8);
BUILTIN_TYPE(Int16);
BUILTIN_TYPE(Int32);
BUILTIN_TYPE(Int64);

BUILTIN_TYPE(Float32);
BUILTIN_TYPE(Float64);

BUILTIN_TYPE(LabelType);
BUILTIN_TYPE(FunctionType);
BUILTIN_TYPE(Pointer);
BUILTIN_TYPE(VoidType);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const Type* Type::Create(TypeID base)
{
	const Type* type = new Type(base);
	return type;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const IntegerType* IntegerType::Create(size_t width)
{
	return new IntegerType(width);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void DestroyType(const Type* type)
{
	delete type;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BuiltinTypes::Init()
{
	s_Int8         = IntegerType::Create(8);
	s_Int16        = IntegerType::Create(16);
	s_Int32        = IntegerType::Create(32);
	s_Int64        = IntegerType::Create(64);
	s_Float32      = Type::Create(kType_Float32);
	s_Float64      = Type::Create(kType_Float64);
	s_LabelType    = Type::Create(kType_LabelType);
	s_FunctionType = Type::Create(kType_FunctionType);
	s_Pointer      = Type::Create(kType_Pointer);
	s_VoidType     = Type::Create(kType_Void);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BuiltinTypes::Destroy()
{
	DestroyType(s_Int8);
	DestroyType(s_Int16);
	DestroyType(s_Int32);
	DestroyType(s_Int64);
	DestroyType(s_Float32);
	DestroyType(s_Float64);
	DestroyType(s_LabelType);
	DestroyType(s_FunctionType);
	DestroyType(s_Pointer);
	DestroyType(s_VoidType);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

