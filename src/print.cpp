#include "print.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const char* Helix::GetOpcodeName(Opcode opcode)
{
	switch (opcode)
	{
	case kInsn_IAdd:       return "iadd";
	case kInsn_ISub:       return "isub";
	case kInsn_IMul:       return "imul";
	case kInsn_IDiv:       return "idiv";
	case kInsn_FAdd:       return "fadd";
	case kInsn_FSub:       return "fsub";
	case kInsn_FMul:       return "fmul";
	case kInsn_FDiv:       return "fdiv";
	case kInsn_And:        return "and";
	case kInsn_Or:         return "or";
	case kInsn_Shl:        return "shl";
	case kInsn_Shr:        return "shr";
	case kInsn_Xor:        return "xor";
	case kInsn_Load:       return "load";
	case kInsn_Store:      return "store";
	case kInsn_StackAlloc: return "stack_alloc";

	case kInsn_Undefined:
	default:
		return "undef";
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Helix::Print(TextOutputStream& out, const Value& value)
{
	if (const ConstantInt* ci = value_cast<ConstantInt>(&value)) {
		out.Write("%llu", ci->GetIntegralValue());
	}
	else if (const VirtualRegisterName* vreg = value_cast<VirtualRegisterName>(&value)) {
		const char* name = vreg->GetDebugName();

		if (name) {
			out.Write("%%%s", name);
		} else {
			out.Write("%%%zu", vreg->GetSlot());
		}
	}

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Helix::Print(TextOutputStream& out, const Instruction& insn)
{
	const char* opcodeName = Helix::GetOpcodeName(insn.GetOpcode());
	out.Write("%s", opcodeName);

	const size_t nOperands = insn.GetCountOperands();

	if (nOperands > 0) {
		out.Write(" ");
	}

	for (size_t i = 0; i < nOperands; ++i) {
		Value* pValue = insn.GetOperand(i);

		if (pValue) {
			Print(out, *pValue);
		}
		else {
			out.Write("?");
		}

		if (i < nOperands - 1) {
			out.Write(", ");
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Helix::Print(TextOutputStream& out, const Function& fn)
{
	const std::string& name = fn.GetName();
	out.Write("function %s() {\n");
	for (const BasicBlock& bb : fn) {
		Print(out, bb);
	}
	out.Write("}\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Helix::Print(TextOutputStream& out, const BasicBlock& bb)
{
	const char* name = bb.GetName();

	if (name) {
		out.Write(".%s", name);
	} else {
		out.Write(".<unnamed>");
	}

	out.Write(":\n");

	for (const Instruction& insn : bb) {
		out.Write("\t");
		Print(out, insn);
		out.Write("\n");
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
