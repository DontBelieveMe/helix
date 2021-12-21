#include "function.h"

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