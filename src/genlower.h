#pragma once

#include "pass-manager.h"

namespace Helix
{
    class BasicBlock;
    class LoadEffectiveAddressInsn;
    class LoadFieldAddressInsn;
    class BinOpInsn;

   	class GenericLowering : public FunctionPass
	{
	public:
		void Execute(Function* fn, const PassRunInformation& info);

	private:
		void LowerLea(BasicBlock& bb, LoadEffectiveAddressInsn& insn);
		void LowerLfa(BasicBlock& bb, LoadFieldAddressInsn& insn);
		void LowerIRem(BasicBlock& bb, BinOpInsn& insn);
	};
}

REGISTER_PASS(GenericLowering, genlower, "[Generic] Lower high level IR constructs");
