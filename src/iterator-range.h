/**
 * @file iterator-range.h
 * @author Barney Wilks
 */

#pragma once

namespace Helix
{
	/**
	 * This wraps two iterators into a interface compatible with C++11s range based for loops.
	 * 
	 * This just makes it nicer to iterate over a range. This is particularly useful
	 * when a class contains multiple sequences that you might want to iterate over (and can't just
	 * expose `begin()`/`end()` on the class and use a for range loop on the object itself.)
	 * 
	 * Equivilant of `llvm::iterator_range` or `boost::iterator_range`
	 * 
	 * ### Example
	 * ```
	 * class A {
	 * public:
	 *     iterator begin() { return ...; }
	 *     iterator end()   { return ...; }
	 * 
	 *     iterator_range<iterator> values() { return iterator_range(begin(), end()); }
	 * };
	 * 
	 * int main() {
	 *     A instance;
	 * 
	 *     for (auto value : instance.values()) {
	 *         // ...
	 *     }
	 * 
	 *     // Instead Of
	 * 
	 *     for (auto it = instance.begin(); it != instance.end(); it++) {
	 *         auto value = *it;
	 *         // ...
	 *     }
	 * }
	 * ```
	 */
	template <typename T>
	class iterator_range
	{
	public:
		iterator_range(const T& range_begin, const T& range_end)
			: m_begin(range_begin), m_end(range_end)
		{ }

		template <typename IterableType>
		iterator_range(const IterableType& iterable)
		    : m_begin(iterable.begin()), m_end(iterable.end())
		{ }

		template <typename IterableType>
		iterator_range(IterableType& iterable)
		    : m_begin(iterable.begin()), m_end(iterable.end())
		{ }

		T begin() const { return m_begin; }
		T end()   const { return m_end;   }

	private:
		T m_begin;
		T m_end;
	};
}
