#pragma once

#include "intrusive_list.h"

namespace Helix
{
	class Instruction;
	
	class BasicBlock : public intrusive_list_node
	{
	private:
		using InstructionList = intrusive_list<Instruction>;

	public:
		using iterator       = InstructionList::iterator;
		using const_iterator = InstructionList::const_iterator;

		iterator       begin()       { return Instructions.begin(); }
		iterator       end()         { return Instructions.end();   }
		const_iterator begin() const { return Instructions.begin(); }
		const_iterator end()   const { return Instructions.end();   }

		iterator InsertBefore(iterator where, Instruction* insn);
		iterator InsertAfter(iterator where, Instruction* insn);

	public:
		InstructionList Instructions;
	};
}
