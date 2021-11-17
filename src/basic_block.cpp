#include "basic_block.h"
#include "instructions.h"

using namespace Helix;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BasicBlock::iterator BasicBlock::InsertBefore(iterator where, Instruction* insn)
{
	return Instructions.insert_before(where, insn);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BasicBlock::iterator BasicBlock::InsertAfter(iterator where, Instruction* insn)
{
	return Instructions.insert_after(where, insn);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
