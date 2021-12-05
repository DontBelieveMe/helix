#include "value.h"
#include "hash.h"
#include "system.h"

#include <unordered_map>
#include <algorithm>

using namespace Helix;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct IntegerSignature
{
	const Type* Ty;
	Integer     Value;

	bool operator==(const IntegerSignature& other) const
		{ return Ty == other.Ty && Value == other.Value; }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace std
{
	template <>
	struct hash<IntegerSignature>
	{
		size_t operator()(const IntegerSignature& sig) const
		{
			size_t hash = std::hash<const Type*>()(sig.Ty);
			Helix::hash_combine(hash, std::hash<Integer>()(sig.Value));
			return hash;
		}
	};
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using IntegerCacheMap = std::unordered_map<IntegerSignature, ConstantInt*>;
using UndefCacheMap = std::unordered_map<const Type*, UndefValue*>;

static IntegerCacheMap s_IntegerCache;
static UndefCacheMap s_UndefCache;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Use::operator==(const Use& other) const
{
	return m_User == other.m_User && m_OperandIndex == other.m_OperandIndex;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

VirtualRegisterName* VirtualRegisterName::Create(const Type* type, const char* name)
{
	VirtualRegisterName* vreg = new VirtualRegisterName(type);
	vreg->m_DebugName = name;
	return vreg;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

VirtualRegisterName* VirtualRegisterName::Create(const Type* type)
{
	return VirtualRegisterName::Create(type, nullptr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ConstantInt* ConstantInt::GetMax(const Type* ty)
{
	const IntegerType* intType = type_cast<IntegerType>(ty);
	helix_assert(intType, "ConstantInt::GetMax(): type is not an integer");

	const uint64_t max = ((uint64_t) 1 << intType->GetBitWidth())  - 1;
	return ConstantInt::Create(ty, max);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ConstantInt* ConstantInt::Create(const Type* ty, Integer value)
{
	IntegerCacheMap::iterator it = s_IntegerCache.find({ty, value});

	if (it != s_IntegerCache.end()) {
		return it->second;
	}

	ConstantInt* ci = new ConstantInt(ty);
	ci->m_Integer   = value;

	const IntegerSignature sig { ty, value };
	s_IntegerCache[sig] = ci;

	return ci;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Value::AddUse(Instruction* user, uint16_t operandIndex)
{
	m_Users.push_back({user, operandIndex});
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Value::RemoveUse(Instruction* user, uint16_t operandIndex)
{
	m_Users.erase(std::remove(m_Users.begin(), m_Users.end(), Use(user, operandIndex)), m_Users.end());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ConstantInt::CanFitInType(const IntegerType* ty) const
{
	// #FIXME(bwilks): This doesn't handle any overflow/underflow cases, and it should

	const IntegerType* myType = type_cast<IntegerType>(this->GetType());

	helix_assert(myType, "Wait, I _really_ should be an integer");

	// If the bit width of the type we want to fit in is bigger,
	// then there are no problems.
	if (ty->GetBitWidth() >= myType->GetBitWidth()) {
		return true;
	}

	// This means that the desired type (ty) is smaller than the
	// type of this integer.
	// We now need to check our actual integral value, to see
	// if it's smaller than the max that the desired type
	// can support.

	const size_t max = ((size_t) 1 << ty->GetBitWidth())  - 1;

	return m_Integer <= max;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UndefValue* UndefValue::Get(const Type* ty)
{
	auto it = s_UndefCache.find(ty);

	if (it == s_UndefCache.end()) {
		UndefValue* v = new UndefValue(ty);
		s_UndefCache.insert({ty, v});
		return v;
	}

	return it->second;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
