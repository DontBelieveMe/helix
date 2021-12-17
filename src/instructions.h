/**
 * @file instructions.h
 * @author Barney Wilks
 */

#pragma once

#include "intrusive-list.h"
#include "value.h"
#include "target-info-armv7.h"
#include "system.h"

#include <vector>
#include <string>

namespace Helix
{
	class BasicBlock;
	class Function;

	using ParameterList = std::vector<Value*>;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	enum Opcode
	{
		#define BEGIN_INSN_CLASS(class_name) kInsnStart_##class_name,
		#define END_INSN_CLASS(class_name) kInsnEnd_##class_name,
		#define DEF_INSN(code_name, pretty_name) kInsn_##code_name,
			#include "insns.def"
	};

#define IMPLEMENT_OPCODE_CATEGORY_IDENTITY(category) \
	constexpr inline bool Is##category(Opcode opc) \
	{ \
		return opc > kInsnStart_##category && opc < kInsnEnd_##category; \
	}

	#define BEGIN_INSN_CLASS(class_name) IMPLEMENT_OPCODE_CATEGORY_IDENTITY(class_name)
	#define END_INSN_CLASS(class_name)
	#define DEF_INSN(code_name, pretty_name)
		#include "insns.def"

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
		    : m_Opcode(opcode)
		{ }

		/**
		 * Set the operand at the given index to 'value'.
		 * 
		 * This instruction is added as a user of the new value, and if there
		 * is a non null value already in the index, this instruction is removed as a user.
		 * If value is null, this clears any operands (and uses) at the current index, nullifying
		 * the operand.
		 * 
		 * Behavior is undefined if index is out of bounds.
		 * 
		 * @param index The index of the operand to set.
		 * @param value The new value, can be null (in which case the operand is cleared).
		 */
		void SetOperand(size_t index, Value* value);
		
		/**
		 * Set a debug comment for this instruction, which gets printed alongside this
		 * instruction for human readable text dumps. Mostly useful for debug & development reasons.
		 * 
		 * @param comment A comment to associate with this instruction.
		 */
		void SetComment(const std::string& comment) { m_DebugComment = comment; }
		
		/**
		 * Return the opcode for this instruction, represented by ::Opcode
		 */
		inline Opcode      GetOpcode()              const { return m_Opcode;                    }
		inline size_t      GetCountOperands()       const { return m_Operands.size();           }
		inline Value*      GetOperand(size_t index) const { return m_Operands[index];           }
		inline std::string GetComment()             const { return m_DebugComment;              }

		inline bool        HasComment()             const { return m_DebugComment.length() > 0; }

		bool IsTerminator() const { return Helix::IsTerminator(m_Opcode); }

		void Clear()
		{
			for (size_t i = 0; i < m_Operands.size(); ++i) {
				SetOperand(i, nullptr);
			}
		}

	protected:
		Opcode      m_Opcode = kInsn_Undefined;
		OperandList m_Operands;
		std::string m_DebugComment;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class BinOpInsn : public Instruction
	{
	public:
		BinOpInsn(Opcode opcode, Value* lhs, Value* rhs, Value* result);

		Value* GetLHS()                  const { return this->GetOperand(0);                                  }
		Value* GetRHS()                  const { return this->GetOperand(1);                                  }
		VirtualRegisterName* GetResult() const { return value_cast<VirtualRegisterName>(this->GetOperand(2)); }
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	class StoreInsn : public Instruction
	{
	public:
		StoreInsn(Value* src, Value* dst);

		Value* GetSrc() const { return this->GetOperand(0); }
		Value* GetDst() const { return this->GetOperand(1); }
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class LoadInsn : public Instruction
	{
	public:
		LoadInsn(Value* src, Value* dst);

		Value* GetSrc() const { return this->GetOperand(0); }
		Value* GetDst() const { return this->GetOperand(1); }
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class StackAllocInsn : public Instruction
	{
	public:
		StackAllocInsn(Value* dst, const Type* type);

		const Type* GetType() const { return m_Type; }

		Value* GetOutputPtr() const { return this->GetOperand(0); }
		const Type* GetAllocatedType() const { return m_Type; }

	private:
		const Type* m_Type;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class ConditionalBranchInsn : public Instruction
	{
	public:
		ConditionalBranchInsn(BasicBlock* trueBB, BasicBlock* falseBB, Value* cond);

		BasicBlock* GetTrueBB() const { return  value_cast<BlockBranchTarget>(this->GetOperand(0))->GetParent(); }
		BasicBlock* GetFalseBB() const { return value_cast<BlockBranchTarget>(this->GetOperand(1))->GetParent(); }
		Value*      GetCond() const { return this->GetOperand(2); }
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class UnconditionalBranchInsn : public Instruction
	{
	public:
		UnconditionalBranchInsn(BasicBlock* bb);

		BasicBlock* GetBB() const { return value_cast<BlockBranchTarget>(this->GetOperand(0))->GetParent(); }
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class CallInsn : public Instruction
	{
	public:
		CallInsn(Function* function, Value* ret, const ParameterList& params);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class RetInsn : public Instruction
	{
	public:
		RetInsn(Value* value);
		RetInsn();

		Value* GetReturnValue() const;
		void MakeVoid();
		bool HasReturnValue() const;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class CompareInsn : public Instruction
	{
	public:
		CompareInsn(Opcode cmpOpcode, Value* lhs, Value* rhs, Value* result);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class LoadEffectiveAddressInsn : public Instruction
	{
	public:
		LoadEffectiveAddressInsn(const Type* baseType, Value* inputPtr, Value* index, Value* outputPtr)
			: Instruction(kInsn_LoadElementAddress, 3), m_Type(baseType)
		{
			this->SetOperand(0, inputPtr);
			this->SetOperand(1, index);
			this->SetOperand(2, outputPtr);
		}

		const Type* GetBaseType() const { return m_Type; }

		Value* GetInputPtr() const { return this->GetOperand(0); }
		Value* GetIndex() const { return this->GetOperand(1); }
		Value* GetOutputPtr() const { return this->GetOperand(2); }

	private:
		const Type* m_Type;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class LoadFieldAddressInsn : public Instruction
	{
	public:
		LoadFieldAddressInsn(const StructType* baseType, Value* inputPtr, unsigned int index, Value* outputPtr)
			: Instruction(kInsn_LoadFieldAddress, 2), m_BaseType(baseType), m_Index(index)
		{
			this->SetOperand(0, inputPtr);
			this->SetOperand(1, outputPtr);
		}

		Value* GetInputPtr() const { return this->GetOperand(0); }
		Value* GetOutputPtr() const { return this->GetOperand(1); }

		const Type* GetBaseType() const { return m_BaseType; }
		unsigned int GetFieldIndex() const { return m_Index; }

	private:
		const Type* m_BaseType;
		unsigned int m_Index;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class CastInsn : public Instruction
	{
	public:
		CastInsn(Opcode opc, const Type* srcTy, const Type* dstTy, Value* in, Value* out)
			: Instruction(opc, 2), m_SrcType(srcTy), m_DstType(dstTy)
		{
			this->SetOperand(0, in);
			this->SetOperand(1, out);
		}

		const Type* GetSrcType() const { return m_SrcType; }
		const Type* GetDstType() const { return m_DstType; }

	private:
		const Type* m_SrcType;
		const Type* m_DstType;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/// Create a comparison instruction that compares 'lhs' and 'rhs' and stores the result to the given
	/// 'result' register.
	CompareInsn* CreateCompare(Opcode cmpOpcode, Value* lhs, Value* rhs, Value* result);

	/// Create a conditional branch that, if the given 'cond' value evaluates to true
	/// jumps to the basic block 'trueBB', and if it's false jump to falseBB.
	ConditionalBranchInsn* CreateConditionalBranch(BasicBlock* trueBB, BasicBlock* falseBB, Value* cond);

	/// Create a unconditional branch that will always jump to the given basic block.
	UnconditionalBranchInsn* CreateUnconditionalBranch(BasicBlock* bb);

	/// Create a call that executes the given function, passing the given list of parameters.
	/// Control flow is returned to the instruction after the call.
	CallInsn* CreateCall(/* Function* fn*/);

	/// Create a return instruction, that returns no value from the current function (void)
	RetInsn* CreateRet();

	/// Create a return instruction that returns a single value from the current function.
	RetInsn* CreateRet(Value* value);

	/// Create a binary operation such that `<op> <lhs>, <rhs>, <result>`
	BinOpInsn* CreateBinOp(Opcode opcode, Value* lhs, Value* rhs, Value* result);

	/// Create a store instruction that stores value 'src' at memory location
	/// given by 'dst' (`store <src>, <dst>`)
	StoreInsn* CreateStore(Value* src, Value* dst);

	/// Create a load instruction that loads a value from the memory address given in 'src'
	/// to the register 'dst' (`load <src>, <dst>`)
	LoadInsn* CreateLoad(Value* src, Value* dst);

	/// Create a stack_alloc instruction that allocates space on the stack and returns
	/// a pointer (memory address) to that space in register 'dst'.
	/// The type of register 'dst' specifies the amount of memory that should be allocated.
	StackAllocInsn* CreateStackAlloc(Value* dst, const Type* type);

	CallInsn* CreateCall(Function* fn, const ParameterList& params);
	CallInsn* CreateCall(Function* fn, Value* returnValue, const ParameterList& params);

	LoadEffectiveAddressInsn* CreateLoadEffectiveAddress(const Type* baseType, Value* input, Value* index, Value* outputPtr);
	LoadFieldAddressInsn* CreateLoadFieldAddress(const StructType* baseType, Value* input, unsigned int index, Value* outputPtr);

	CastInsn* CreatePtrToInt(const Type* dstIntType, Value* inputPtr, Value* outputInt);
	CastInsn* CreateIntToPtr(const Type* srcIntType, Value* inputInt, Value* outputPtr);

	void DestroyInstruction(Instruction* insn);

	inline std::string stringify_operand(Value* v)
	{
		if (ConstantInt* ci = value_cast<ConstantInt>(v)) {
			return std::to_string(ci->GetIntegralValue());
		}

		if (PhysicalRegisterName* preg = value_cast<PhysicalRegisterName>(v)) {
			return PhysicalRegisters::GetRegisterString((PhysicalRegisters::ArmV7RegisterID) preg->GetID());
		}

		helix_unimplemented("stringify_operand, unknown value type");
		return {};
	}

	inline bool is_const_int_with_value(Value* v, Integer i)
	{
		ConstantInt* ci = value_cast<ConstantInt>(v);
		if (!ci)
			return false;
		return ci->GetIntegralValue() == i;
	}

	inline bool is_int(Value* v)
	{
		return value_isa<ConstantInt>(v);
	}

	inline bool is_register(Value* v)
	{
		return value_isa<PhysicalRegisterName>(v);
	}
}
