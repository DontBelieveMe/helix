#pragma once

#include "pass-manager.h"

namespace Helix
{
	class StackAllocInsn;

	class BasicBlock;
	class RegisterAllocator : public FunctionPass
	{
	public:
		virtual void Execute(Function* fn, const PassRunInformation& info) override;

	private:
		struct StackVariable
		{
			/// Size in bytes.
			unsigned        size;

			/// Offset from the stack pointer.
			unsigned        offset;

			/// stack_alloc instruction that "allocates" this variable.
			StackAllocInsn* alloca_insn;
		};

		struct StackFrame
		{
			/// List of each variable/"individual allocations" in the stack frame.
			std::vector<StackVariable> variables;

			/// Amount of space we need to allocate (in bytes).
			/// Might not match the sum of the sizes of each variable, owing to
			/// alignment requirements.
			unsigned size;
		};

		void ComputeStackFrame(StackFrame& frame, Function* function);
	};

}

REGISTER_PASS(RegisterAllocator, regalloc, "[ARM] Register allocation");