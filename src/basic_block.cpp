#include "basic_block.h"
#include "instructions.h"
#include "types.h"

using namespace Helix;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BasicBlock* BasicBlock::Create(const char* name)
{
	BasicBlock* bb = new BasicBlock();
	bb->Name = name;
	return bb;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BasicBlock* BasicBlock::Create()
{
	return BasicBlock::Create(nullptr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BasicBlock::BasicBlock()
	: BranchTarget(this), Value(kValue_BasicBlock, BuiltinTypes::GetLabelType())
{ }

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
