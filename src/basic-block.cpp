///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File: basic-block.cpp
//
// This file implements BasicBlock, defined in basic_block.h
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "basic-block.h"
#include "instructions.h"
#include "types.h"
#include "mir.h"

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
	
	return insn.IsTerminator();
}

/*********************************************************************************************************************/

const Instruction* BasicBlock::GetTerminator() const
{
	if (!HasTerminator())
		return nullptr;

	return &Instructions.back();
}

/*********************************************************************************************************************/

template <typename T>
static bool MapContainsValue(const T& it, const T& mapend)
{
	return it != mapend;
}

/*********************************************************************************************************************/

std::set<VirtualRegisterName*> BasicBlock::CalculateDefs()
{
	// The 'def' set of a basic block (B) is defined as:
	//
	//   > "The set of variables defined (i.e., definitely assigned values) in B
	//   > prior to any use of that variable in B"...
	//
	// Compilers: Principles, Techniques, & Tools (2nd ed.). Pearson.
	//     Aho, A. V., Lam, M. S., Sethi, R., & Ullman, J. D. (2007).
	//
 	// (Page 634 [PDF], Page 609 [Headings])

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

					// Only want definitions _prior_ to any use - e.g.
					// shouldn't exist in the usedVariables set
					if (!MapContainsValue(it, usedVariables.end())) {
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
	// The 'use' set of a basic block (B) is defined as:
	//
	//   > "The set of variables whose values may be used in B prior to any
	//   > definition of the variable"
	//
	// Compilers: Principles, Techniques, & Tools (2nd ed.). Pearson.
	//     Aho, A. V., Lam, M. S., Sethi, R., & Ullman, J. D. (2007).
	//
	// (Page 634 [PDF], Page 609 [Headings])


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

					// Only want uses _prior_ to any definitions, e.g.
					// shouldn't exist in the definitions set.
					if (!MapContainsValue(it, definedVariables.end())) {
						usedVariables.insert(operand);
					}
				}
			}
		}
	}

	return usedVariables;
}

/*********************************************************************************************************************/

std::vector<BasicBlock*> BasicBlock::GetSuccessors() const
{
	std::vector<BasicBlock*> successors;

	for (const Instruction& insn : Instructions) {
		if (insn.IsTerminator()) {
			for (size_t i = 0; i < insn.GetCountOperands(); ++i) {
				if (BlockBranchTarget* v = value_cast<BlockBranchTarget>(insn.GetOperand(i))) {
					successors.push_back(v->GetParent());
				}
			}
		}
	}

	return successors;
}

/*********************************************************************************************************************/
