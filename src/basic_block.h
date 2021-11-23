#pragma once

#include "intrusive_list.h"
#include "value.h"
#include "instructions.h"

namespace Helix
{
	class BasicBlock : public intrusive_list_node, public Value
	{
	private:
		using InstructionList = intrusive_list<Instruction>;

		BasicBlock();

	public:
		using iterator       = InstructionList::iterator;
		using const_iterator = InstructionList::const_iterator;

		using insn_iterator       = iterator;
		using const_insn_iterator = const_iterator;

		iterator       begin()       { return Instructions.begin(); }
		iterator       end()         { return Instructions.end();   }
		const_iterator begin() const { return Instructions.begin(); }
		const_iterator end()   const { return Instructions.end();   }

		iterator InsertBefore(iterator where, Instruction* insn);
		iterator InsertAfter(iterator where, Instruction* insn);

		static BasicBlock* Create(const char* name);
		static BasicBlock* Create();

		const char* GetName() const { return Name; }

		BlockBranchTarget* GetBranchTarget() { return &BranchTarget; }

		bool HasTerminator() const;

		bool HasComment() const { return Comment.length() > 0; }
		void SetComment(const std::string& comment) { Comment = comment; }
		std::string GetComment() const { return Comment; }

	private:
		InstructionList Instructions;
		const char*     Name;
		BlockBranchTarget BranchTarget;
		std::string Comment;
	};
}
