#pragma once

#include "intrusive_list.h"
#include "value.h"

namespace Helix
{
	class Instruction;
	
	class BasicBlock : public Value
	{
	private:
		using InstructionList = intrusive_list<Instruction>;

		BasicBlock();

	public:
		using iterator       = InstructionList::iterator;
		using const_iterator = InstructionList::const_iterator;

		iterator       begin()       { return Instructions.begin(); }
		iterator       end()         { return Instructions.end();   }
		const_iterator begin() const { return Instructions.begin(); }
		const_iterator end()   const { return Instructions.end();   }

		iterator InsertBefore(iterator where, Instruction* insn);
		iterator InsertAfter(iterator where, Instruction* insn);

		static BasicBlock* Create(const char* name);
		static BasicBlock* Create();

		const char* GetName() const { return Name; }

	private:
		InstructionList Instructions;
		const char*     Name;
	};

	IMPLEMENT_VALUE_TRAITS(BasicBlock, kValue_BasicBlock);
}
