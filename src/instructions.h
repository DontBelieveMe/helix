#pragma once

#include "intrusive_list.h"
#include "value.h"

#include <vector>

namespace Helix
{
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	enum Opcode
	{
		kInsnStart_BinaryOps,
			kInsn_IAdd,
			kInsn_ISub,
			kInsn_IMul,
			kInsn_IDiv,
			kInsn_FAdd,
			kInsn_FSub,
			kInsn_FMul,
			kInsn_FDiv,

			kInsn_And,
			kInsn_Or,
			kInsn_Shl,
			kInsn_Shr,
			kInsn_Xor,
		kInsnEnd_BinaryOps,

		kInsn_Undefined
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class Instruction : public intrusive_list_node
	{
	public:
		Instruction(Opcode opcode, size_t nOperands)
			: m_Opcode(opcode)
		{
			m_Operands.resize(nOperands);
		}

		Instruction(Opcode opcode)
			: m_Opcode(opcode) { }

		Opcode GetOpcode() const { return m_Opcode; }

		inline size_t GetCountOperands() const { return m_Operands.size(); }
		inline Value* GetOperand(size_t index) const { return m_Operands[index]; }

	protected:
		Opcode              m_Opcode = kInsn_Undefined;
		std::vector<Value*> m_Operands;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class BinOp : public Instruction
	{
	public:
		BinOp(Opcode opcode, Value* lhs, Value* rhs, VirtualRegisterName* result);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	BinOp* CreateBinaryOp(Opcode opcode, Value* lhs, Value* rhs, VirtualRegisterName* result);
}
