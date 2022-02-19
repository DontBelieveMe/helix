///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// File: basic-block.h
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// This file defines BasicBlock, a linearly executing sequence of instructions containing no branches
// except one (a terminator) at the end.
//
// BasicBlocks are composed as part of an intrusive linked list into functions. One basic block is unique to its
// parent function, and cannot be shared over multiple functions. The branch instruction at the end of the
// basic block is known as its "terminator".
//
// Well formed basic blocks, must (as per above) contain a terminator instruction, and no intermediate branches
// (except call, which is not a terminating branch since control flow returns to the caller). Because of this,
// basic blocks with no instructions are ill formed.
//
// BasicBlocks must be created with the static functions BasicBlock::Create(...), and once finished with
// destroyed with BasicBlock::Destroy(...). Trying to destroy a basic block that is still in use, or is
// not empty is invalid (note that due to this, basic blocks must be ill formed just before destruction).
//
// BasicBlocks do not inherit Value - Instructions that need to reference this basic block as an operand
// (e.g. branches) should instead use the BlockBranchTarget* given by BasicBlock::GetBranchTarget() instead (which
// does implement Value).
//
// BasicBlock should not be used as a value type & should only be interacted with via pointer.
// To this aim the copy & move constructors & assignment operators have been deleted.
//
// Implementation is in basic_block.cpp
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>
#include <set>

#include <stddef.h>

#include "intrusive-list.h"
#include "instructions.h"
#include "value.h"
#include "system.h"
#include "iterator-range.h"

namespace Helix
{
	class Function;

	class BasicBlock : public intrusive_list_node
	{
	private:
		using InstructionList = intrusive_list<Instruction>;

		// Deliberately private, since we want external users to use the static
		// Create & Destroy functions.
		BasicBlock();

	public:
		HELIX_NO_STEAL(BasicBlock);

		using iterator            = InstructionList::iterator;
		using const_iterator      = InstructionList::const_iterator;

		using insn_iterator       = iterator;
		using const_insn_iterator = const_iterator;

		iterator       begin()       { return Instructions.begin(); }
		iterator       end()         { return Instructions.end();   }
		const_iterator begin() const { return Instructions.begin(); }
		const_iterator end()   const { return Instructions.end();   }

		iterator InsertBefore(iterator where, Instruction* insn);
		iterator InsertAfter(iterator where, Instruction* insn);
		void     Append(Instruction* insn);

		Instruction* GetLast() { return &Instructions.back(); }

		static BasicBlock* Create(const char* name);
		static BasicBlock* Create();

		static void Destroy(BasicBlock* block);

		void SetComment(const std::string& comment) { Comment = comment; }

		bool HasTerminator() const;
		bool HasComment()    const { return Comment.length() > 0; }
		bool IsEmpty()       const { return Instructions.empty(); }

		const Instruction* GetTerminator() const;

		std::string        GetComment()      const { return Comment;                     }
		size_t             GetCountUses()    const { return BranchTarget.GetCountUses(); }
		const char*        GetName()         const { return Name;                        }
		BlockBranchTarget* GetBranchTarget()       { return &BranchTarget;               }

		void Replace(Instruction* original, Instruction* newValue)
		{
			newValue->SetParent(this);
			original->SetParent(nullptr);

			Instructions.replace(original, newValue);
		}

		std::set<VirtualRegisterName*>& GetLiveIn() { return LiveIn; }
		std::set<VirtualRegisterName*>& GetLiveOut() { return LiveOut; }

		const std::set<VirtualRegisterName*>& GetLiveIn() const { return LiveIn;  }
		const std::set<VirtualRegisterName*>& GetLiveOut() const { return LiveOut; }

		std::vector<BasicBlock*> GetSuccessors() const;

		void Remove(iterator where)
		{
			where->SetParent(nullptr);
			Instructions.remove(where);
		}

		bool CanDelete() const;

		std::set<VirtualRegisterName*> CalculateUses();
		std::set<VirtualRegisterName*> CalculateDefs();

		iterator_range<iterator> insns() { return iterator_range(begin(), end()); }

		iterator Where(Instruction* insn) const { return iterator(insn); }
		const_iterator Where(const Instruction* insn) const { return const_iterator(insn); }

		size_t GetCountInstructions() const {
			return Instructions.size();
		}

		Function* GetParent() const { return Parent; }
		void SetParent(Function* fn) { Parent = fn; }

	private:
		InstructionList   Instructions;
		const char*       Name;
		BlockBranchTarget BranchTarget;
		std::string       Comment;
		Function*         Parent = nullptr;

		std::set<VirtualRegisterName*> LiveIn;
		std::set<VirtualRegisterName*> LiveOut;
	};
}
