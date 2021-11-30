#pragma once

#include "intrusive-list.h"
#include "basic-block.h"

#include <string>

namespace Helix
{
	class Function
	{
		using BlockList = intrusive_list<BasicBlock>;
		using ParamList = std::vector<Value*>;

	public:
		using iterator             = BlockList::iterator;
		using const_iterator       = BlockList::const_iterator;

		using block_iterator       = iterator;
		using const_block_iterator = const_iterator;

		using param_iterator       = ParamList::iterator;
		using const_param_iterator = ParamList::const_iterator;

		iterator       begin()       { return m_Blocks.begin(); }
		iterator       end()         { return m_Blocks.end();   }
		const_iterator begin() const { return m_Blocks.begin(); }
		const_iterator end()   const { return m_Blocks.end();   }

		param_iterator       params_begin()       { return m_Parameters.begin(); }
		param_iterator       params_end()         { return m_Parameters.end();   }
		const_param_iterator params_begin() const { return m_Parameters.begin(); }
		const_param_iterator params_end()   const { return m_Parameters.end();   }

		static Function* Create(const std::string& name, const Type* returnType)
		{
			Function* fn = new Function();
			fn->m_Name = name;
			fn->m_ReturnType = returnType;
			return fn;
		}

		iterator InsertBefore(iterator where, BasicBlock* bb);
		iterator InsertAfter(iterator where, BasicBlock* bb);

		void Remove(iterator where);

		const Type* GetReturnType() const { return m_ReturnType; }

		bool IsVoidReturn() const { return m_ReturnType == BuiltinTypes::GetVoidType(); }

		inline std::string GetName() const { return m_Name; }

		size_t GetCountBlocks() const { return m_Blocks.size(); }

		void AddParameter(Value* v)
		{
			m_Parameters.push_back(v);
		}

	private:
		BlockList   m_Blocks;
		const Type* m_ReturnType;
		std::string m_Name;
		ParamList   m_Parameters;
	};
}
