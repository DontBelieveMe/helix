/* Internal Project Includes */
#include "function.h"

/* C++ Standard Library Includes */
#include <algorithm>
#include <iterator>

using namespace Helix;

/******************************************************************************/

Function::iterator Function::InsertBefore(iterator where, BasicBlock* what)
{
	what->SetParent(this);
	return m_Blocks.insert_before(where, what);
}

/******************************************************************************/

Function::iterator Function::InsertAfter(iterator where, BasicBlock* what)
{
	what->SetParent(this);
	return m_Blocks.insert_after(where, what);
}

/******************************************************************************/

void Function::Append(BasicBlock* bb)
{
	bb->SetParent(this);
	m_Blocks.push_back(bb);
}

/******************************************************************************/

void Function::Remove(iterator where)
{
	where->SetParent(nullptr);
	m_Blocks.remove(where);
}

/******************************************************************************/

BasicBlock* Function::GetTailBlock()
{
	if (m_Blocks.empty()) {
		return nullptr;
	}

	return &m_Blocks.back();
}

/******************************************************************************/

BasicBlock* Function::GetHeadBlock()
{
	if (m_Blocks.empty()) {
		return nullptr;
	}

	return &m_Blocks.front();
}

/******************************************************************************/

using LiveMap = std::unordered_map<BasicBlock*, std::set<VirtualRegisterName*>>;

/******************************************************************************/

// Return true if the IN set changes for this basic block
static bool ComputeLiveInForBlock(BasicBlock* bb)
{
	// IN[B] = B.Uses UNION (OUT[B] DIFFERENCE B.Defs)

	const std::set<VirtualRegisterName*>& blockLiveOut = bb->GetLiveOut();
	std::set<VirtualRegisterName*>&       blockLiveIn  = bb->GetLiveIn();

	// 1) Calculate the 'use' set for B
	const std::set<VirtualRegisterName*> uses = bb->CalculateUses();

	// 2) Calculate the 'def' set for B
	const std::set<VirtualRegisterName*> defs = bb->CalculateDefs();

	// 3) Then do the OUT[B] DIFFERENCE B.Defs operation
	//    (calculating the difference between the OUT set for B and
	//    'def' set for B)
	std::set<VirtualRegisterName*> difference;
	std::set_difference(blockLiveOut.begin(), blockLiveOut.end(),
	                   defs.begin(), defs.end(),
					   std::inserter(difference, difference.begin()));

	// #FIXME (bwilks): To check if the set changes we make a copy of the
	//                  IN set before the union so we can compare it to
	//                  the IN set after the union. This seems really
	//                  quite poor...
	const std::set<VirtualRegisterName*> old = blockLiveIn;

	// 4) Finalize the IN set for B (clear first so that we're overwriting
	//    the old IN set with the new values)
	blockLiveIn.clear();
	std::set_union(uses.begin(), uses.end(),
	               difference.begin(), difference.end(),
				   std::inserter(blockLiveIn, blockLiveIn.begin()));

	return blockLiveIn != old;
}

/******************************************************************************/

static void ComputeLiveOutForBlock(BasicBlock* bb)
{
	// OUT[B] = UNION of the IN set for each successor of B

	std::set<VirtualRegisterName*>& blockLiveOut = bb->GetLiveOut();
	blockLiveOut.clear();

	const std::vector<BasicBlock*> successors = bb->GetSuccessors();

	for (BasicBlock* successor : successors) {
		const std::set<VirtualRegisterName*>&
			successorLiveIn = successor->GetLiveIn();

		std::set_union(blockLiveOut.begin(), blockLiveOut.end(),
		               successorLiveIn.begin(), successorLiveIn.end(),
		               std::inserter(blockLiveOut, blockLiveOut.begin()));
	}
}

/******************************************************************************/

void Function::RunLivenessAnalysis()
{
	int nIterations = 0;

	// Continuously iterate while there are changes to IN[bb]
	for (;;) {
		bool dirty = false;

		// For each block...
		for (BasicBlock& bb : m_Blocks) {

			// Compute OUT[bb]
			ComputeLiveOutForBlock(&bb);

			// Compute IN[bb] and if IN[bb] changes then we need to
			// run another iteration (since OUT[bb] is dependent on IN[bb]
			// we'll need to recompute it when IN[bb] changes)
			if (ComputeLiveInForBlock(&bb)) {
				dirty = true;
			}
		}

		nIterations++;

		if (!dirty) {
			break;
		}
	}

	helix_debug(logs::general, "Liveness analysis finished in {} iterations",
			nIterations);
}

/******************************************************************************/

Function*
Function::Create(const FunctionType* type, const std::string& name,
    const ParamList& params)
{
	Function* fn = new Function(type);

	fn->m_Name       = name;
	fn->m_Parameters = params;

	return fn;
}

/******************************************************************************/

Value*
Function::GetParameter(size_t index) const
{
	if (index >= m_Parameters.size())
		return nullptr;

	return m_Parameters[index];
}

/******************************************************************************/

size_t
Function::GetCountParameters() const
{
	return m_Parameters.size();
}

/******************************************************************************/

bool
Function::HasBody() const
{
	return !m_Blocks.empty();
}

/******************************************************************************/

void
Function::SetParent(Module* parent)
{
	m_Parent = parent;
}

/******************************************************************************/

Module*
Function::GetParent() const
{
	return m_Parent;
}

/******************************************************************************/

bool
Function::IsVoidReturn() const
{
	return GetReturnType() == BuiltinTypes::GetVoidType();
}

/******************************************************************************/

const Type*
Function::GetReturnType() const
{
	const FunctionType* type = type_cast<FunctionType>(GetType());

	if (!type)
		return nullptr;

	return type->GetReturnType();
}

/******************************************************************************/

std::string
Function::GetName() const
{
	return m_Name;
}

/******************************************************************************/

size_t
Function::GetCountBlocks() const
{
	return m_Blocks.size();
}

/******************************************************************************/

bool
Helix::is_function(Value* v)
{
	return Helix::value_isa<Function>(v);
}

/******************************************************************************/
