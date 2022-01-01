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

	class LowerStructStackAllocation : public FunctionPass
	{
	public:
		void Execute(Function* fn);
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
}

/*********************************************************************************************************************/

REGISTER_PASS(LowerStructStackAllocation, lowerallocastructs, "[Generic] Lower the stack allocations of structs to arrays");
REGISTER_PASS(ConstantHoisting, consthoist, "[ARM] Split and hoist constants to a form compatible with the ARM");
REGISTER_PASS(ReturnCombine, retcomb, "[Generic] Combine multiple returns into a singular exit point");
REGISTER_PASS(CConv, cconv, "[ARM] Lower IR to be compatible with the platform calling convention");
REGISTER_PASS(GenericLegalizer, genlegal, "[Generic] Legalise illegal constructs IR to a legal equivilant");
REGISTER_PASS(GenericLowering, genlower, "[Generic] Lower high level IR constructs");

/*********************************************************************************************************************/
