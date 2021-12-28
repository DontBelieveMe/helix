#pragma once

#include "pass-manager.h"

/*********************************************************************************************************************/

namespace Helix
{
	/*********************************************************************************************************************/

	class LoadEffectiveAddressInsn;
	class LoadFieldAddressInsn;
	class BasicBlock;
	class StoreInsn;
	class GlobalVariable;
	class Module;
	class ConstantInt;
	class StackAllocInsn;
	class BinOpInsn;

	/*********************************************************************************************************************/

	class GenericLegalizer : public FunctionPass
	{
	public:
		void Execute(Function* fn);

	private:
		void LegaliseStore(BasicBlock& bb, StoreInsn& store);
	};

	/*********************************************************************************************************************/

	class GenericLowering : public FunctionPass
	{
	public:
		void Execute(Function* fn);

	private:
		void LowerLea(BasicBlock& bb, LoadEffectiveAddressInsn& insn);
		void LowerLfa(BasicBlock& bb, LoadFieldAddressInsn& insn);
		void LowerIRem(BasicBlock& bb, BinOpInsn& insn);
	};

	/*********************************************************************************************************************/

	class CConv : public FunctionPass
	{
	public:
		void Execute(Function* fn);
	};

	/*********************************************************************************************************************/

	class ReturnCombine : public FunctionPass
	{
	public:
		void Execute(Function* fn);
	};

	/*********************************************************************************************************************/

	class ConstantHoisting : public BasicBlockPass
	{
	public:
		void Execute(BasicBlock* bb);

	private:
		GlobalVariable* CreateOrGetGlobal(Module* bb, ConstantInt* cint);

	private:
		std::unordered_map<ConstantInt*, GlobalVariable*> GlobalMap;
	};

	/*********************************************************************************************************************/

	class LowerStackAllocations : public FunctionPass
	{
	public:
		void Execute(Function* fn);

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

	/*********************************************************************************************************************/
}

/*********************************************************************************************************************/

REGISTER_PASS(LowerStackAllocations, allocalower, "[ARM] Lower stack_alloc instructions into the lower level arithmetic");
REGISTER_PASS(ConstantHoisting, consthoist, "[ARM] Split and hoist constants to a form compatible with the ARM");
REGISTER_PASS(ReturnCombine, retcomb, "[Generic] Combine multiple returns into a singular exit point");
REGISTER_PASS(CConv, cconv, "[ARM] Lower IR to be compatible with the platform calling convention");
REGISTER_PASS(GenericLegalizer, genlegal, "[Generic] Legalise illegal constructs IR to a legal equivilant");
REGISTER_PASS(GenericLowering, genlower, "[Generic] Lower high level IR constructs");

/*********************************************************************************************************************/
