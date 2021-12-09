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

static const Type* InternalCreateType(TypeID base)
{
	return new Type(base);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const IntegerType* IntegerType::Create(size_t width)
{
	return new IntegerType(width);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void InternalDestroyType(const Type* type)
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

	s_Float32      = InternalCreateType(kType_Float32);
	s_Float64      = InternalCreateType(kType_Float64);
	s_LabelType    = InternalCreateType(kType_LabelType);
	s_FunctionType = InternalCreateType(kType_FunctionType);
	s_Pointer      = InternalCreateType(kType_Pointer);
	s_VoidType     = InternalCreateType(kType_Void);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BuiltinTypes::Destroy()
{
	InternalDestroyType(s_Int8);
	InternalDestroyType(s_Int16);
	InternalDestroyType(s_Int32);
	InternalDestroyType(s_Int64);
	InternalDestroyType(s_Float32);
	InternalDestroyType(s_Float64);
	InternalDestroyType(s_LabelType);
	InternalDestroyType(s_FunctionType);
	InternalDestroyType(s_Pointer);
	InternalDestroyType(s_VoidType);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

