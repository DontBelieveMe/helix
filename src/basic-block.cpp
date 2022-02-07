///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File: basic-block.cpp
//
// This file implements BasicBlock, defined in basic_block.h
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "basic-block.h"
#include "instructions.h"
#include "types.h"

using namespace Helix;

/*********************************************************************************************************************/

BasicBlock* BasicBlock::Create(const char* name)
{
	BasicBlock* bb = new BasicBlock();
	bb->Name = name;
	return bb;
}

/*********************************************************************************************************************/

BasicBlock* BasicBlock::Create()
{
	return BasicBlock::Create(nullptr);
}

/*********************************************************************************************************************/

bool BasicBlock::CanDelete() const
{
	return GetCountUses() == 0 && Instructions.empty();
}

/*********************************************************************************************************************/

void BasicBlock::Destroy(BasicBlock* bb)
{
	helix_assert(bb->BranchTarget.GetCountUses() == 0, "Cannot delete BB, outstanding uses");
	helix_assert(bb->Instructions.empty(), "Cannot delete BB, it's not empty");
	
	delete bb;
}

/*********************************************************************************************************************/

BasicBlock::BasicBlock()
	: BranchTarget(this), Name(NULL)
{ }

/*********************************************************************************************************************/

BasicBlock::iterator BasicBlock::InsertBefore(iterator where, Instruction* insn)
{
	insn->SetParent(this);
	return Instructions.insert_before(where, insn);
}

/*********************************************************************************************************************/

BasicBlock::iterator BasicBlock::InsertAfter(iterator where, Instruction* insn)
{
	insn->SetParent(this);
	return Instructions.insert_after(where, insn);
}

/*********************************************************************************************************************/

bool BasicBlock::HasTerminator() const
{
	if (Instructions.empty())
		return false;

	const Instruction& insn = Instructions.back();
	return Helix::IsTerminator(insn.GetOpcode());
}

/*********************************************************************************************************************/

const Instruction* BasicBlock::GetTerminator() const
{
	if (!HasTerminator())
		return nullptr;

	return &Instructions.back();
}

/*********************************************************************************************************************/

std::set<VirtualRegisterName*> BasicBlock::CalculateDefs()
{
	std::set<VirtualRegisterName*> definedVariables;
	std::set<VirtualRegisterName*> usedVariables;

	for (Instruction& insn : this->Instructions) {
		for (size_t i = 0; i < insn.GetCountOperands(); ++i) {
			if (VirtualRegisterName* operand = value_cast<VirtualRegisterName>(insn.GetOperand(i))) {
				if (insn.OperandHasFlags(i, Instruction::OP_READ)) {
					usedVariables.insert(operand);
				}
				else if (insn.OperandHasFlags(i, Instruction::OP_WRITE)) {
					auto it = usedVariables.find(operand);

					if (it != usedVariables.end()) {
						definedVariables.insert(operand);
					}
				}
			}
		}
	}

	return definedVariables;
}

/*********************************************************************************************************************/

std::set<VirtualRegisterName*> BasicBlock::CalculateUses()
{
	std::set<VirtualRegisterName*> usedVariables;
	std::set<VirtualRegisterName*> definedVariables;

	for (Instruction& insn : this->Instructions) {
		for (size_t i = 0; i < insn.GetCountOperands(); ++i) {
			if (VirtualRegisterName* operand = value_cast<VirtualRegisterName>(insn.GetOperand(i))) {
				if (insn.OperandHasFlags(i, Instruction::OP_WRITE)) {
					definedVariables.insert(operand);
				}
				else if (insn.OperandHasFlags(i, Instruction::OP_READ)) {
					auto it = definedVariables.find(operand);

					if (it != definedVariables.end()) {
						usedVariables.insert(operand);
					}
				}
			}
		}
	}

	return usedVariables;
}

/*********************************************************************************************************************/
