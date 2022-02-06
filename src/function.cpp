#include "function.h"

#include <algorithm>
#include <iterator>

using namespace Helix;

Function::iterator Function::InsertBefore(iterator where, BasicBlock* what)
{
	what->SetParent(this);
	return m_Blocks.insert_before(where, what);
}

Function::iterator Function::InsertAfter(iterator where, BasicBlock* what)
{
	what->SetParent(this);
	return m_Blocks.insert_after(where, what);
}

void Function::Remove(iterator where)
{
	where->SetParent(nullptr);
	m_Blocks.remove(where);
}

BasicBlock* Function::GetTailBlock()
{
	if (m_Blocks.empty()) {
		return nullptr;
	}

	return &m_Blocks.back();
}

/*********************************************************************************************************************/

BasicBlock* Function::GetHeadBlock()
{
	if (m_Blocks.empty()) {
		return nullptr;
	}

	return &m_Blocks.front();
}

/*********************************************************************************************************************/

using LiveMap = std::unordered_map<BasicBlock*, std::set<VirtualRegisterName*>>;

static void ComputeLiveInForBlock(BasicBlock* bb)
{
	std::set<VirtualRegisterName*>& blockLiveOut = bb->GetLiveOut();

	const std::set<VirtualRegisterName*> uses = bb->CalculateUses();
	const std::set<VirtualRegisterName*> defs = bb->CalculateDefs();

	std::set<VirtualRegisterName*> difference;

	std::set_difference(blockLiveOut.begin(), blockLiveOut.end(),
	                   defs.begin(), defs.end(),
					   std::inserter(difference, difference.begin()));

	blockLiveOut.clear();

	std::set_union(uses.begin(), uses.end(),
	               difference.begin(), difference.end(),
				   std::inserter(blockLiveOut, blockLiveOut.begin()));
}

/*********************************************************************************************************************/

static std::set<VirtualRegisterName*> ComputeLiveOut(BasicBlock* bb)
{
	
}

/*********************************************************************************************************************/

void Function::RunLivenessAnalysis()
{

}

/*********************************************************************************************************************/
