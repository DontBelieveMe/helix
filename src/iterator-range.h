#pragma once

namespace Helix
{
	template <typename T>
	class iterator_range
	{
	public:
		iterator_range(const T& range_begin, const T& range_end)
			: m_begin(range_begin), m_end(range_end)
		{ }

		T begin() const { return m_begin; }
		T end()   const { return m_end;   }

	private:
		T m_begin;
		T m_end;
	};
}