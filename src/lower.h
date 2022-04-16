#pragma once

#include "pass-manager.h"
#include "basic-block.h"

/*********************************************************************************************************************/

namespace Helix
{
	/*********************************************************************************************************************/

	class LoadEffectiveAddressInsn;
	class LoadFieldAddressInsn;
	class StoreInsn;
	class GlobalVariable;
	class Module;
	class ConstantInt;
	class StackAllocInsn;
	class BinOpInsn;
	class Value;

	/*********************************************************************************************************************/

	class GenericLegalizer : public FunctionPass
	{
	public:
		void Execute(Function* fn, const PassRunInformation& info);

	private:
		void LegaliseStore(BasicBlock& bb, StoreInsn& store);
	};

	/*********************************************************************************************************************/

	class LowerStructStackAllocation : public FunctionPass
	{
	public:
		void Execute(Function* fn, const PassRunInformation& info);
	};

	/*********************************************************************************************************************/

	class LegaliseStructs : public FunctionPass
	{
	public:
		void Execute(Function* fn, const PassRunInformation& info);

	private:
		void CopyStruct(Value* src, Value* dst, const StructType* structType, BasicBlock::iterator where);
	};

	/*********************************************************************************************************************/

	class CConv : public FunctionPass
	{
	public:
		void Execute(Function* fn, const PassRunInformation& info);
	};

	/*********************************************************************************************************************/

	class ReturnCombine : public FunctionPass
	{
	public:
		void Execute(Function* fn, const PassRunInformation& info);
	};

	/*********************************************************************************************************************/
}

/*********************************************************************************************************************/

REGISTER_PASS(LegaliseStructs, structslegal, "[Generic] Legalise loading & storing struct types to/from virtual registers");
REGISTER_PASS(LowerStructStackAllocation, lowerallocastructs, "[Generic] Lower the stack allocations of structs to arrays");
REGISTER_PASS(ReturnCombine, retcomb, "[Generic] Combine multiple returns into a singular exit point");
REGISTER_PASS(CConv, cconv, "[ARM] Lower IR to be compatible with the platform calling convention");
REGISTER_PASS(GenericLegalizer, genlegal, "[Generic] Legalise illegal constructs IR to a legal equivilant");

/*********************************************************************************************************************/
