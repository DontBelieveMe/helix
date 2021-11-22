#include "instructions.h"
#include "basic_block.h"

using namespace Helix;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CompareInsn* Helix::CreateCompare(Opcode cmpOpcode, Value* lhs, Value* rhs, VirtualRegisterName* result)
{
	return new CompareInsn(cmpOpcode, lhs, rhs, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BinOpInsn* Helix::CreateBinOp(Opcode opcode, Value* lhs, Value* rhs, VirtualRegisterName* result)
{
	return new BinOpInsn(opcode, lhs, rhs, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UnconditionalBranchInsn* Helix::CreateUnconditionalBranch(BasicBlock* bb)
{
	return new UnconditionalBranchInsn(bb);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ConditionalBranchInsn* Helix::CreateConditionalBranch(BasicBlock* trueBB, BasicBlock* falseBB, Value* cond)
{
	return new ConditionalBranchInsn(trueBB, falseBB, cond);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

StoreInsn* Helix::CreateStore(Value* src, VirtualRegisterName* dst)
{
	return new StoreInsn(src, dst);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LoadInsn* Helix::CreateLoad(VirtualRegisterName* src, VirtualRegisterName* dst)
{
	return new LoadInsn(src, dst);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

StackAllocInsn* Helix::CreateStackAlloc(VirtualRegisterName* dst)
{
	return new StackAllocInsn(dst);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RetInsn* Helix::CreateRet()
{
	return new RetInsn();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RetInsn* Helix::CreateRet(Value* value)
{
	return new RetInsn(value);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ConditionalBranchInsn::ConditionalBranchInsn(BasicBlock* trueBB, BasicBlock* falseBB, Value* cond)
	: Instruction(kInsn_Cbr, 3)
{
	m_Operands[0] = trueBB->GetBranchTarget();
	m_Operands[1] = falseBB->GetBranchTarget();
	m_Operands[2] = cond;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UnconditionalBranchInsn::UnconditionalBranchInsn(BasicBlock* bb)
	: Instruction(kInsn_Br, 1)
{
	m_Operands[0] = bb->GetBranchTarget();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BinOpInsn::BinOpInsn(Opcode opcode, Value* lhs, Value* rhs, VirtualRegisterName* result)
	: Instruction(opcode, 3)
{
	m_Operands[0] = lhs;
	m_Operands[1] = rhs;
	m_Operands[2] = result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

StoreInsn::StoreInsn(Value* src, VirtualRegisterName* dst)
	: Instruction(kInsn_Store, 2)
{
	m_Operands[0] = src;
	m_Operands[1] = dst;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LoadInsn::LoadInsn(VirtualRegisterName* src, VirtualRegisterName* dst)
	: Instruction(kInsn_Load, 2)
{
	m_Operands[0] = src;
	m_Operands[1] = dst;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

StackAllocInsn::StackAllocInsn(VirtualRegisterName* dst)
	: Instruction(kInsn_StackAlloc, 1)
{
	m_Operands[0] = dst;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
