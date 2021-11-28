#pragma once

#include <functional>

namespace Helix
{
	/// Call repeatedly to combine the hash of 'val' with 'seed'.
	/// Taken from boost::hash_combine
	///    https://www.boost.org/doc/libs/1_77_0/doc/html/hash/reference.html#boost.hash_combine
	///
	/// Might not be the best way of doing this (giving the best distribution), but it works
	/// well enough for what I want for now.
	template<typename T>
	inline void hash_combine (size_t& seed, const T& val)
	{
		seed ^= std::hash<T>()(val) + 0x9e3779b9 + (seed<<6) + (seed>>2);
	}
}
