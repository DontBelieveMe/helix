#include "function.h"

using namespace Helix;

Function::iterator Function::InsertBefore(iterator where, BasicBlock* what)
{
	return m_Blocks.insert_before(where, what);
}

Function::iterator Function::InsertAfter(iterator where, BasicBlock* what)
{
	return m_Blocks.insert_after(where, what);
}
