/**
 * @file function.h
 * @author Barney Wilks
 */

#pragma once

/* Internal Project Includes */
#include "intrusive-list.h"
#include "basic-block.h"
#include "iterator-range.h"

/* C++ Standard Library Includes */
#include <string>

/******************************************************************************/

namespace Helix
{
	/**************************************************************************/

	class Module;

	/**************************************************************************/

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


		static Function* Create(const FunctionType* type,
				                const std::string& name,
								const ParamList& params);

		void RunLivenessAnalysis();

		iterator InsertBefore(iterator where, BasicBlock* bb);
		iterator InsertAfter(iterator where, BasicBlock* bb);
		void Append(BasicBlock* bb);

		void Remove(iterator where);

		/// Get the last basic block in the function.
		BasicBlock* GetTailBlock();

		/// Get the first basic block in the function.
		BasicBlock* GetHeadBlock();

		/// Return true if this function returns void, false
		/// if not.
		bool IsVoidReturn() const;

		/// Return true if this function has at least
		/// one basic block, false if no basic blocks are in
		/// the function.
		bool HasBody() const;

		/// Get the return type of this function.
		const Type* GetReturnType() const;

		/// Return the name of this function (ASCII, not mangled)
		std::string GetName() const;

		/// Return the number of basic blocks in this function
		size_t GetCountBlocks() const;

		/// Get the parent module for this function. Null if
		/// by default (i.e. not set with SetParent)
		Module* GetParent() const;

		/// Return number of parameter values (not necessarily the
		/// same as the number of parameter types, but should be...)
		size_t GetCountParameters() const;

		/// Get the parameter value at the given index. If index
		/// is out of bounds, null is returned.
		Value* GetParameter(size_t index) const;

		/// Set the parent module for this function. Originally null
		/// if nothing has been already set.
		void SetParent(Module* parent);

		iterator       begin()       { return m_Blocks.begin(); }
		iterator       end()         { return m_Blocks.end();   }
		const_iterator begin() const { return m_Blocks.begin(); }
		const_iterator end()   const { return m_Blocks.end();   }

		param_iterator params_begin()
			{ return m_Parameters.begin(); }

		param_iterator params_end()
			{ return m_Parameters.end(); }

		const_param_iterator params_begin() const
			{ return m_Parameters.begin(); }

		const_param_iterator params_end() const
			{ return m_Parameters.end(); }

		iterator_range<param_iterator> params()
			{ return iterator_range(params_begin(), params_end()); }

		iterator Where(BasicBlock* bb)
			{ return iterator(bb); }

		const_iterator Where(BasicBlock* bb) const
			{ return const_iterator(bb); }

		iterator_range<block_iterator> blocks()
			{ return iterator_range(begin(), end()); }

		iterator_range<const_block_iterator> blocks() const
			{ return iterator_range(begin(), end()); }

	private:
		BlockList    m_Blocks;
		ParamList    m_Parameters;
		std::string  m_Name;
		Module*      m_Parent = nullptr;
	};

	/**************************************************************************/

	IMPLEMENT_VALUE_TRAITS(Function, kValue_Function);

	/**************************************************************************/

	bool is_function(Value* v);
}

/******************************************************************************/
