#pragma once

#include "intrusive-list.h"
#include "basic-block.h"
#include "iterator-range.h"

#include <string>

namespace Helix
{
	class Function : public Value
	{
	public:
		Function(const FunctionType* ty)
			: Value(kValue_Function, ty) { }

		using BlockList = intrusive_list<BasicBlock>;
		using ParamList = std::vector<Value*>;

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

		static Function* Create(const FunctionType* type, const std::string& name, const ParamList& params)
		{
			Function* fn = new Function(type);

			fn->m_Name       = name;
			fn->m_Parameters = params;

			return fn;
		}

		iterator InsertBefore(iterator where, BasicBlock* bb);
		iterator InsertAfter(iterator where, BasicBlock* bb);

		void Remove(iterator where);

		bool               IsVoidReturn()  const { return GetReturnType() == BuiltinTypes::GetVoidType();     }
		const Type*        GetReturnType() const { return ((const FunctionType*) GetType())->GetReturnType(); }
		inline std::string GetName()       const { return m_Name;                                             }

		size_t GetCountBlocks() const { return m_Blocks.size(); }

		iterator_range<block_iterator> blocks() { return iterator_range(begin(), end()); }

	private:
		BlockList    m_Blocks;
		ParamList    m_Parameters;
		std::string  m_Name;
	};

	IMPLEMENT_VALUE_TRAITS(Function, kValue_Function);
}
