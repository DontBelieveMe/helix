#include "instructions.h"
#include "basic-block.h"
#include "function.h"
#include "print.h"
#include "mir.h"
#include "ir-helpers.h"

using namespace Helix;

static std::vector<Instruction::OperandFlags> s_fixedOperandsFlags[HLIR::kInsnCount] =
{
	#define DEF_INSN_FIXED(code_name,pretty_name, n_operands, ...) { __VA_ARGS__ },

	// Need to define these (even though they're not used) so we can use the opcode
	// directly as an index into the table & since the opcode enum has these in them
	// if we didn't have them in the table here the indexes would all be wrong
	#define DEF_INSN_DYN(code_name,pretty_name) { },
	#define BEGIN_INSN_CLASS(class_name) { },
	#define END_INSN_CLASS(class_name) { },

		#include "insns.def"
} ;

/*********************************************************************************************************************/

TruncInsn* Helix::CreateTruncInsn(Value* oldValue, Value* newValue)
{
	return new TruncInsn(oldValue, newValue);
}

/*********************************************************************************************************************/

SetInsn* Helix::CreateSetInsn(Value* reg, Value* newValue)
{
	return new SetInsn(reg, newValue);
}

/*********************************************************************************************************************/

CastInsn* Helix::CreateSExt(Value* input, Value* output)
{
	return new CastInsn(HLIR::SExt, input, output);
}

/*********************************************************************************************************************/

CastInsn* Helix::CreateZExt(Value* input, Value* output)
{
	return new CastInsn(HLIR::ZExt, input, output);
}

/*********************************************************************************************************************/

CastInsn* Helix::CreatePtrToInt(Value* inputPtr, Value* outputInt)
{
	return new CastInsn(HLIR::PtrToInt, inputPtr, outputInt);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CastInsn* Helix::CreateIntToPtr(Value* inputInt, Value* outputPtr)
{
	return new CastInsn(HLIR::IntToPtr, inputInt, outputPtr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LoadFieldAddressInsn* Helix::CreateLoadFieldAddress(const StructType* baseType, Value* input, unsigned int index, Value* outputPtr)
{
	return new LoadFieldAddressInsn(baseType, input, index, outputPtr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LoadEffectiveAddressInsn* Helix::CreateLoadEffectiveAddress(const Type* baseType, Value* input, Value* index, Value* outputPtr)
{
	return new LoadEffectiveAddressInsn(baseType, input, index, outputPtr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CallInsn* Helix::CreateCall(Function* fn, const ParameterList& params)
{
	return new CallInsn(fn, UndefValue::Get(fn->GetReturnType()), params);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CallInsn* Helix::CreateCall(Function* fn, Value* returnValue, const ParameterList& params)
{
	return new CallInsn(fn, returnValue, params);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CompareInsn* Helix::CreateCompare(HLIR::Opcode cmpOpcode, Value* lhs, Value* rhs, Value* result)
{
	return new CompareInsn(cmpOpcode, lhs, rhs, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BinOpInsn* Helix::CreateBinOp(HLIR::Opcode opcode, Value* lhs, Value* rhs, Value* result)
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

StoreInsn* Helix::CreateStore(Value* src, Value* dst)
{
	return new StoreInsn(src, dst);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LoadInsn* Helix::CreateLoad(Value* src, Value* dst)
{
	return new LoadInsn(src, dst);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

StackAllocInsn* Helix::CreateStackAlloc(Value* dst, const Type* type)
{
	return new StackAllocInsn(dst, type);
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

void Instruction::SetOperand(size_t index, Value* value)
{
	helix_assert(index < UINT16_MAX, "index too big");

	if (m_Operands[index] != nullptr) {
		m_Operands[index]->RemoveUse(this, (uint16_t) index);
	}

	m_Operands[index] = value;

	if (value) {
		value->AddUse(this, (uint16_t) index);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CallInsn::CallInsn(Function* function, Value* ret, const ParameterList& params)
	: Instruction(HLIR::Call)
{
	m_Operands.resize(params.size() + 2, nullptr);

	this->SetOperand(0, ret);
	this->SetOperand(1, function);

	for (size_t i = 2; i < m_Operands.size(); ++i) {
		this->SetOperand(i, params[i - 2]);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ConditionalBranchInsn::ConditionalBranchInsn(BasicBlock* trueBB, BasicBlock* falseBB, Value* cond)
	: Instruction(HLIR::ConditionalBranch, 3)
{
	this->SetOperand(0, trueBB->GetBranchTarget());
	this->SetOperand(1, falseBB->GetBranchTarget());
	this->SetOperand(2, cond);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UnconditionalBranchInsn::UnconditionalBranchInsn(BasicBlock* bb)
	: Instruction(HLIR::UnconditionalBranch, 1)
{
	this->SetOperand(0, bb->GetBranchTarget());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BinOpInsn::BinOpInsn(HLIR::Opcode opcode, Value* lhs, Value* rhs, Value* result)
	: Instruction(opcode, 3)
{
	this->SetOperand(0, lhs);
	this->SetOperand(1, rhs);
	this->SetOperand(2, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

StoreInsn::StoreInsn(Value* src, Value* dst)
	: Instruction(HLIR::Store, 2)
{
	this->SetOperand(0, src);
	this->SetOperand(1, dst);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LoadInsn::LoadInsn(Value* src, Value* dst)
	: Instruction(HLIR::Load, 2)
{
	this->SetOperand(0, src);
	this->SetOperand(1, dst);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

StackAllocInsn::StackAllocInsn(Value* dst, const Type* type)
	: Instruction(HLIR::StackAlloc, 1), m_Type(type)
{
	this->SetOperand(0, dst);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CompareInsn::CompareInsn(HLIR::Opcode cmpOpcode, Value* lhs, Value* rhs, Value* result)
	: Instruction(cmpOpcode, 3)
{
	this->SetOperand(0, lhs);
	this->SetOperand(1, rhs);
	this->SetOperand(2, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RetInsn::RetInsn(Value* value)
	: Instruction(HLIR::Return, 1)
{
	this->SetOperand(0, value);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RetInsn::RetInsn()
	: Instruction(HLIR::Return, 0)
{ }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool RetInsn::HasReturnValue() const
{
	return GetCountOperands() == 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Value* RetInsn::GetReturnValue() const
{
	if (!HasReturnValue())
		return nullptr;

	return GetOperand(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RetInsn::MakeVoid()
{
	if (HasReturnValue()) {
		// Clear out the operand with SetOperand(..., nullptr) to remove this instruction
		// from the use list of the value.
		this->SetOperand(0, nullptr);

		// Finally clear the operand list itself (0 operands = no return value, aka void)
		m_Operands.clear();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string Helix::stringify_operand(Value* v, SlotTracker& slots)
{
	if (!v)
		return "(null)";

	if (ConstantInt* ci = value_cast<ConstantInt>(v)) {
		return std::to_string(ci->GetIntegralValue());
	}
	if (PhysicalRegisterName* preg = value_cast<PhysicalRegisterName>(v)) {
		return PhysicalRegisters::GetRegisterString((PhysicalRegisters::ArmV7RegisterID) preg->GetID());
	}
	if (GlobalVariable* gvar = value_cast<GlobalVariable>(v)) {
		return std::string(gvar->GetName());
	}
	if (BlockBranchTarget* bb = value_cast<BlockBranchTarget>(v)) {
		const size_t slot = slots.GetBasicBlockSlot(bb->GetParent());
		return fmt::format(".bb{}", slot);
	}
	if (VirtualRegisterName* vreg = value_cast<VirtualRegisterName>(v)) {
		return fmt::format("%{}", slots.GetValueSlot(v));
	}
	if (Function* fn = value_cast<Function>(v)) {
		return fn->GetName();
	}

	helix_unimplemented("stringify_operand, unknown value type");
	return {};
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static Instruction::OperandFlags GetFixedOpFlags(const Instruction* insn, size_t index)
{
	if (index >= insn->GetCountOperands())
		return Instruction::OP_NONE;

	return s_fixedOperandsFlags[insn->GetOpcode()][index];
}

Instruction::OperandFlags BinOpInsn::GetOperandFlags(size_t i) const { return GetFixedOpFlags(this, i); }
Instruction::OperandFlags StoreInsn::GetOperandFlags(size_t i) const { return GetFixedOpFlags(this, i); }
Instruction::OperandFlags LoadInsn::GetOperandFlags(size_t i) const { return GetFixedOpFlags(this, i); }
Instruction::OperandFlags StackAllocInsn::GetOperandFlags(size_t i) const { return GetFixedOpFlags(this, i); }
Instruction::OperandFlags ConditionalBranchInsn::GetOperandFlags(size_t i) const { return GetFixedOpFlags(this, i); }
Instruction::OperandFlags UnconditionalBranchInsn::GetOperandFlags(size_t i) const { return GetFixedOpFlags(this, i); }
Instruction::OperandFlags CompareInsn::GetOperandFlags(size_t i) const { return GetFixedOpFlags(this, i); }
Instruction::OperandFlags LoadEffectiveAddressInsn::GetOperandFlags(size_t i) const { return GetFixedOpFlags(this, i); }
Instruction::OperandFlags LoadFieldAddressInsn::GetOperandFlags(size_t i) const { return GetFixedOpFlags(this, i); }
Instruction::OperandFlags CastInsn::GetOperandFlags(size_t i) const { return GetFixedOpFlags(this, i); }
Instruction::OperandFlags SetInsn::GetOperandFlags(size_t i) const { return GetFixedOpFlags(this, i);  }
Instruction::OperandFlags TruncInsn::GetOperandFlags(size_t i) const { return GetFixedOpFlags(this, i); }

Instruction::OperandFlags CallInsn::GetOperandFlags(size_t i) const
{
	if (i >= this->GetCountOperands()) {
		return Instruction::OP_NONE;
	}

	switch (i) {
	case 0:  return Instruction::OP_WRITE;
	case 1:  return Instruction::OP_READ;
	default: return Instruction::OP_READ;
	}
}

Instruction::OperandFlags RetInsn::GetOperandFlags(size_t i) const
{
	if (i >= this->GetCountOperands()) {
		return Instruction::OP_NONE;
	}

	return Instruction::OP_READ;
}

void Instruction::DeleteFromParent()
{
	// helix_assert(m_Parent, "can't delete instruction from parent since parent is null");
	IR::DestroyInstruction(this);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Instruction::IsTerminator() const
{
	if (Helix::IsMachineOpcode(m_Opcode))
		return Helix::IsMachineTerminator(m_Opcode);

	return HLIR::IsTerminator((HLIR::Opcode) m_Opcode);
}
