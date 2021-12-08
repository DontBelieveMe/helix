#pragma once

#include "intrusive-list.h"
#include "basic-block.h"

#include <string>

namespace Helix
{
	class Function
	{
	public:
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

		static Function* Create(const std::string& name, const Type* returnType, const ParamList& params)
		{
			Function* fn = new Function();
			
			FunctionDef::ParamTypeList paramTypes;
			paramTypes.reserve(params.size());
			for (Value* v : params) {
				paramTypes.push_back(v->GetType());
			}

			fn->m_Parameters = params;
			fn->m_Def = FunctionDef::Create(name, returnType, paramTypes);

			return fn;
		}

		iterator InsertBefore(iterator where, BasicBlock* bb);
		iterator InsertAfter(iterator where, BasicBlock* bb);

		void Remove(iterator where);

		FunctionDef* GetDefinition() const { return m_Def; }
		const Type* GetReturnType() const { return m_Def->GetReturnType(); }
		bool IsVoidReturn() const { return GetReturnType() == BuiltinTypes::GetVoidType(); }
		inline std::string GetName() const { return m_Def->GetName(); }

		size_t GetCountBlocks() const { return m_Blocks.size(); }

	private:
		FunctionDef* m_Def;
		BlockList    m_Blocks;
		ParamList    m_Parameters;
	};
}
