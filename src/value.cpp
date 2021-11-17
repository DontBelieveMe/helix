#include "value.h"

using namespace Helix;

VirtualRegisterName* VirtualRegisterName::Create(const char* name)
{
	VirtualRegisterName* vreg = new VirtualRegisterName();
	vreg->m_DebugName = name;
	return vreg;
}

VirtualRegisterName* VirtualRegisterName::Create()
{
	return VirtualRegisterName::Create(nullptr);
}

ConstantInt* ConstantInt::Create(const Type* ty, Integer value)
{
	ConstantInt* ci = new ConstantInt();
	ci->m_Type    = ty;
	ci->m_Integer = value;
	return ci;
}
