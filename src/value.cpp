#include "value.h"
#include "hash.h"

#include <unordered_map>

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

static IntegerCacheMap s_IntegerCache;
static size_t          s_VirtualRegisterSlot = 1;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

VirtualRegisterName* VirtualRegisterName::Create(const Type* type, const char* name)
{
	VirtualRegisterName* vreg = new VirtualRegisterName(type);
	vreg->m_DebugName = name;
	vreg->m_Slot      = s_VirtualRegisterSlot++;
	return vreg;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

VirtualRegisterName* VirtualRegisterName::Create(const Type* type)
{
	return VirtualRegisterName::Create(type, nullptr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ConstantInt* ConstantInt::Create(const Type* ty, Integer value)
{
	IntegerCacheMap::iterator it = s_IntegerCache.find({ty, value});

	if (it != s_IntegerCache.end())
		return it->second;

	ConstantInt* ci = new ConstantInt(ty);
	ci->m_Integer   = value;

	const IntegerSignature sig { ty, value };
	s_IntegerCache[sig] = ci;

	return ci;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
