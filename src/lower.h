#include "pass-manager.h"

namespace Helix
{
	class LoadEffectiveAddressInsn;
	class LoadFieldAddressInsn;
	class BasicBlock;

	class GenericLowering : public FunctionPass
	{
	public:
		void Execute(Function* fn);

	private:
		void Lower_Lea(BasicBlock& bb, LoadEffectiveAddressInsn& insn);
		void Lower_Lfa(BasicBlock& bb, LoadFieldAddressInsn& insn);
	};
}