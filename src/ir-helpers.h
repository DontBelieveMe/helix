#pragma once

#include "basic-block.h"

namespace Helix::IR
{
	static void ReplaceAllUsesWith(Value* oldValue, Value* newValue)
	{
		std::vector<Use> worklist;

		for (auto it = oldValue->uses_begin(); it != oldValue->uses_end(); ++it) {
			worklist.push_back(*it);
		}

		for (Use& use : worklist) {
			use.ReplaceWith(newValue);
		}
	}

	template <typename T>
	struct ParentedInsn
	{
		T& insn;
		BasicBlock& parent;
	};

	template <typename T>
	static void BuildWorklist(std::vector<ParentedInsn<T>>& insns, Function* fn, OpcodeType opcode)
	{
		for (BasicBlock& bb : fn->blocks()) {
			for (Instruction& insn : bb.insns()) {
				if (insn.GetOpcode() == opcode) {
					insns.push_back({ static_cast<T&>(insn), bb });
				}
			}
		}
	}

	template <typename T>
	static T* FindFirstInstructionOfType(BasicBlock& bb, OpcodeType opcode) {
		for (Instruction& insn : bb) {
			if (insn.GetOpcode() == opcode) {
				return static_cast<T*>(&insn);
			}
		}

		return nullptr;
	}
}