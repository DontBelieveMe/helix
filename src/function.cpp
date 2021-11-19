#include "function.h"

using namespace Helix;

Function::iterator Function::InsertBefore(iterator where, BasicBlock* what)
{
	return m_Blocks.insert_before(where, what);
}
