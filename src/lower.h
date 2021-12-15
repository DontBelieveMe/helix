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

	REGISTER_PASS(CConv, "cconv", "");
	REGISTER_PASS(GenericLegalizer, "genlegal", "");
	REGISTER_PASS(GenericLowering,  "genlower", "");
}