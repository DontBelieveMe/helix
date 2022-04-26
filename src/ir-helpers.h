/**
 * @file ir-helpers.h
 * @author Barney Wilks
 * 
 * Various utilities & helpers for accessing & manipulating
 * any of the IR forms (HLIR, LLIR).
 */

#pragma once

/* Internal Project Includes */
#include "basic-block.h"
#include "intrusive-list.h"

/******************************************************************************/

namespace Helix::IR
{
	void ReplaceAllUsesWith(Value* oldValue, Value* newValue);

	template <typename T>
	struct ParentedInsn
	{
		T& insn;
		BasicBlock& parent;
	};

	void DestroyInstruction(Instruction* insn);

	/**
	 * Replace instruction a with instruction b.
	 * 
	 * 'b' no longer then gets reparented with its new parent
	 * being the previous parent of 'a'.
	 * 
	 * 'a' gets destroyed (deleted), make sure no references remain :)
	 */
	void ReplaceInstructionAndDestroyOriginal(Instruction* a, Instruction* b);

	void ReplaceInstructionAndPreserveOriginal(Instruction* a, Instruction* b);

	/**
	 * Insert instruction 'b' just before 'a' (reparenting 'b' under the parent
	 * block of 'a')
	 */
	void InsertBefore(Instruction* a, Instruction* b);

	/**
	 * Insert instruction 'b' just after 'a' (reparenting 'b' under the parent
	 * block of 'a')
	 */
	void InsertAfter(Instruction* a, Instruction* b);

	bool TryGetSingleUser(Instruction* base, Value* v, Use* outUse);

	inline BasicBlock::iterator GetNext(Instruction* insn) {
		return BasicBlock::iterator((Instruction*) insn->get_next());
	}

	inline BasicBlock::iterator GetPrev(Instruction* insn) {
		return BasicBlock::iterator((Instruction*)insn->get_prev());
	}

	size_t GetCountReadUsers(Value* v);
	size_t GetCountWriteUsers(Value* v);

	template <typename T>
	inline void BuildWorklist(std::vector<ParentedInsn<T>>& insns,
	                          Function* fn, OpcodeType opcode);

	template <typename T>
	inline void FindAllInstructionsOfType(std::vector<T*>& insns,
	                                      BasicBlock* bb, OpcodeType opcode);

	template <typename T>
	inline T* FindFirstInstructionOfType(BasicBlock& bb, OpcodeType opcode);

	std::vector<BasicBlock*> GetPredecessors(BasicBlock* bb);

	class InsnSeq
	{
		using List = intrusive_list<Instruction>;

	public:
		using iterator       = List::iterator;
		using const_iterator = List::const_iterator;

		void Append(Instruction* insn);

		Instruction* GetHead();
		Instruction* GetTail();

		size_t GetSize() const;
		void Clear();

		iterator begin() { return m_Instructions.begin(); }
		iterator end()   { return m_Instructions.end(); }

		const_iterator begin() const { return m_Instructions.begin(); }
		const_iterator end()   const { return m_Instructions.end();   }

	private:
		List m_Instructions;
	};

	void ReplaceInstructionAndDestroyOriginal(Instruction* a, InsnSeq& b);
}

/******************************************************************************/

template <typename T>
inline void Helix::IR::FindAllInstructionsOfType(std::vector<T*>& insns,
    BasicBlock* bb, OpcodeType opcode)
{
	for (Instruction& insn : *bb) {
		if (insn.GetOpcode() == opcode) {
			insns.push_back(static_cast<T*>(&insn));
		}
	}
}

/******************************************************************************/

template <typename T>
inline T* Helix::IR::FindFirstInstructionOfType(BasicBlock& bb,
    OpcodeType opcode) {

	for (Instruction& insn : bb) {
		if (insn.GetOpcode() == opcode) {
			return static_cast<T*>(&insn);
		}
	}

	return nullptr;
}

/******************************************************************************/

template <typename T>
inline void Helix::IR::BuildWorklist(std::vector<ParentedInsn<T>>& insns,
    Function* fn, OpcodeType opcode)
{
	for (BasicBlock& bb : fn->blocks()) {
		for (Instruction& insn : bb.insns()) {
			if (insn.GetOpcode() == opcode) {
				insns.push_back({ static_cast<T&>(insn), bb });
			}
		}
	}
}

/******************************************************************************/
