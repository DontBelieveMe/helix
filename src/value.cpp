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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

VirtualRegisterName* VirtualRegisterName::Create(const char* name)
{
	VirtualRegisterName* vreg = new VirtualRegisterName();
	vreg->m_DebugName = name;
	return vreg;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

VirtualRegisterName* VirtualRegisterName::Create()
{
	return VirtualRegisterName::Create(nullptr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ConstantInt* ConstantInt::Create(const Type* ty, Integer value)
{
	IntegerCacheMap::iterator it = s_IntegerCache.find({ty, value});

	if (it != s_IntegerCache.end())
		return it->second;

	ConstantInt* ci = new ConstantInt();
	ci->m_Type      = ty;
	ci->m_Integer   = value;

	const IntegerSignature sig { ty, value };
	s_IntegerCache[sig] = ci;

	return ci;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
