#pragma once

#include <functional>

namespace Helix
{
	template<typename T>
	inline void hash_combine (size_t& seed, const T& val)
	{
		seed ^= std::hash<T>()(val) + 0x9e3779b9 + (seed<<6) + (seed>>2);
	}
}
