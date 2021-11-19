#pragma once

#include "intrusive_list.h"
#include "basic_block.h"

#include <string>

namespace Helix
{
	class Function
	{
		using BlockList = intrusive_list<BasicBlock>;

	public:
		using iterator       = BlockList::iterator;
		using const_iterator = BlockList::const_iterator;

		iterator       begin()       { return m_Blocks.begin(); }
		iterator       end()         { return m_Blocks.end();   }
		const_iterator begin() const { return m_Blocks.begin(); }
		const_iterator end()   const { return m_Blocks.end();   }

		static Function* Create(const std::string& name)
		{
			Function* fn = new Function();
			fn->m_Name = name;
			return fn;
		}

		iterator InsertBefore(iterator where, BasicBlock* bb);

		inline std::string GetName() const { return m_Name; }

	private:
		BlockList   m_Blocks;
		std::string m_Name;
	};
}