#include "pass-manager.h"

namespace Helix
{
	class LoadEffectiveAddressInsn;
	class LoadFieldAddressInsn;
	class BasicBlock;
	class StoreInsn;

	class GenericLegalizer : public FunctionPass
	{
	public:
		void Execute(Function* fn);

	private:
		void LegaliseStore(BasicBlock& bb, StoreInsn& store);
	};

	class GenericLowering : public FunctionPass
	{
	public:
		void Execute(Function* fn);

	private:
		void LowerLea(BasicBlock& bb, LoadEffectiveAddressInsn& insn);
		void LowerLfa(BasicBlock& bb, LoadFieldAddressInsn& insn);
	};

	class CConv : public FunctionPass
	{
	public:
		void Execute(Function* fn);
	};

	class ReturnCombine : public FunctionPass
	{
	public:
		void Execute(Function* fn);
	};
}

REGISTER_PASS(ReturnCombine, retcomb, "[Generic] Combine multiple returns into a singular exit point");
REGISTER_PASS(CConv, cconv, "[ARM] Lower IR to be compatible with the platform calling convention");
REGISTER_PASS(GenericLegalizer, genlegal, "[Generic] Legalise illegal constructs IR to a legal equivilant");
REGISTER_PASS(GenericLowering, genlower, "[Generic] Lower high level IR constructs");
