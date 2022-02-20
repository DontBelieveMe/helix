/**
 * @file peephole-generic.h
 * @author Barney Wilks
 */

#include "pass-manager.h"
#include "basic-block.h"

namespace Helix
{
	class Instruction;

	class PeepholeGeneric : public FunctionPass
	{
	public:
		void Execute(Function* fn, const PassRunInformation& info) override;

	private:
		BasicBlock::iterator DoInstruction(BasicBlock::iterator input, bool* bFlagChanges);
		BasicBlock::iterator DoIMul(BinOpInsn* imul, bool* bFlagChanges);
		BasicBlock::iterator DoGenericBinOp(BinOpInsn* binop, bool* bFlagChanges);
	};
}

REGISTER_PASS(PeepholeGeneric, peepholegeneric, "[Generic] Generic peephole optimisations");
