#pragma once

#include "intrusive_list.h"
#include "value.h"

#include <vector>
#include <string>

namespace Helix
{
	class BasicBlock;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	enum Opcode
	{
		kInsnStart_BinaryOp,
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
		kInsnEnd_BinaryOp,

		kInsn_Load,
		kInsn_Store,
		kInsn_StackAlloc,

		kInsnStart_Branch,
			kInsnStart_Terminator,
				kInsn_Cbr,
				kInsn_Br,
				kInsn_Ret,
			kInsnEnd_Terminator,

			kInsn_Call,
		kInsnEnd_Branch,

		kInsnStart_Compare,
			kInsn_FCmp_Neq,
			kInsn_FCmp_Eq,
			kInsn_FCmp_Gt,
			kInsn_FCmp_Lt,
			kInsn_FCmp_Gte,
			kInsn_FCmp_Lte,

			kInsn_ICmp_Neq,
			kInsn_ICmp_Eq,
			kInsn_ICmp_Gt,
			kInsn_ICmp_Lt,
			kInsn_ICmp_Gte,
			kInsn_ICmp_Lte,

		kInsnEnd_Compare,

		kInsn_Undefined,
	};

#define IMPLEMENT_OPCODE_CATEGORY_IDENTITY(category) \
	constexpr inline bool Is##category(Opcode opc) \
	{ \
		return opc > kInsnStart_##category && opc < kInsnEnd_##category; \
	}

	IMPLEMENT_OPCODE_CATEGORY_IDENTITY(Compare)
	IMPLEMENT_OPCODE_CATEGORY_IDENTITY(BinaryOp)
	IMPLEMENT_OPCODE_CATEGORY_IDENTITY(Branch)
	IMPLEMENT_OPCODE_CATEGORY_IDENTITY(Terminator)

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class Instruction : public intrusive_list_node
	{
		using OperandList = std::vector<Value*>;

	public:
		virtual ~Instruction() { }

		Instruction(Opcode opcode, size_t nOperands)
			: m_Opcode(opcode)
		{
			m_Operands.resize(nOperands, nullptr);
		}

		Instruction(Opcode opcode)
			: m_Opcode(opcode) { }

		Opcode GetOpcode() const { return m_Opcode; }

		void SetOperand(size_t index, Value* value);

		inline size_t GetCountOperands() const { return m_Operands.size(); }
		inline Value* GetOperand(size_t index) const { return m_Operands[index]; }

		void SetComment(const std::string& comment) { m_DebugComment = comment; }
		std::string GetComment() const { return m_DebugComment; }

		bool HasComment() const { return m_DebugComment.length() > 0; }

	protected:
		Opcode      m_Opcode = kInsn_Undefined;
		OperandList m_Operands;
		std::string m_DebugComment;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class BinOpInsn : public Instruction
	{
	public:
		BinOpInsn(Opcode opcode, Value* lhs, Value* rhs, VirtualRegisterName* result);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	class StoreInsn : public Instruction
	{
	public:
		StoreInsn(Value* src, VirtualRegisterName* dst);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class LoadInsn : public Instruction
	{
	public:
		LoadInsn(VirtualRegisterName* src, VirtualRegisterName* dst);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class StackAllocInsn : public Instruction
	{
	public:
		StackAllocInsn(VirtualRegisterName* dst, const Type* type, size_t count);

		inline const Type* GetType()  const { return m_Type;  }
		inline size_t      GetCount() const { return m_Count; }

	private:
		const Type*  m_Type  = nullptr;
		size_t       m_Count = 0;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class ConditionalBranchInsn : public Instruction
	{
	public:
		ConditionalBranchInsn(BasicBlock* trueBB, BasicBlock* falseBB, Value* cond);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class UnconditionalBranchInsn : public Instruction
	{
	public:
		UnconditionalBranchInsn(BasicBlock* bb);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class CallInsn : public Instruction
	{
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class RetInsn : public Instruction
	{
	public:
		RetInsn(Value* value);
		RetInsn();
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class CompareInsn : public Instruction
	{
	public:
		CompareInsn(Opcode cmpOpcode, Value* lhs, Value* rhs, VirtualRegisterName* result);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/// Create a comparison instruction that compares 'lhs' and 'rhs' and stores the result to the given
	/// 'result' register.
	CompareInsn* CreateCompare(Opcode cmpOpcode, Value* lhs, Value* rhs, VirtualRegisterName* result);

	/// Create a conditional branch that, if the given 'cond' value evaluates to true
	/// jumps to the basic block 'trueBB', and if it's false jump to falseBB.
	ConditionalBranchInsn* CreateConditionalBranch(BasicBlock* trueBB, BasicBlock* falseBB, Value* cond);

	/// Create a unconditional branch that will always jump to the given basic block.
	UnconditionalBranchInsn* CreateUnconditionalBranch(BasicBlock* bb);

	/// Create a call that executes the given function, passing the given list of parameters.
	/// Contrl flow is returned to the instruction after the call.
	CallInsn* CreateCall(/* Function* fn*/);

	/// Create a return instruction, that returns no value from the current function (void)
	RetInsn* CreateRet();

	/// Create a return instruction that returns a single value from the current function.
	RetInsn* CreateRet(Value* value);

	/// Create a binary operation such that `<op> <lhs>, <rhs>, <result>`
	BinOpInsn* CreateBinOp(Opcode opcode, Value* lhs, Value* rhs, VirtualRegisterName* result);

	/// Create a store instruction that stores value 'src' at memory location
	/// given by 'dst' (`store <src>, <dst>`)
	StoreInsn* CreateStore(Value* src, VirtualRegisterName* dst);

	/// Create a load instruction that loads a value from the memory address given in 'src'
	/// to the register 'dst' (`load <src>, <dst>`)
	LoadInsn* CreateLoad(VirtualRegisterName* src, VirtualRegisterName* dst);

	/// Create a stack_alloc instruction that allocates space on the stack and returns
	/// a pointer (memory address) to that space in register 'dst'.
	/// The type of register 'dst' specifies the amount of memory that should be allocated.
	StackAllocInsn* CreateStackAlloc(VirtualRegisterName* dst, const Type* type, size_t count);
}
