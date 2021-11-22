#include "print.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static const char* kColour_Keyword  = Helix::TextOutputStream::kColour_Green;
static const char* kColour_Number   = Helix::TextOutputStream::kColour_Cyan;
static const char* kColour_Typename = Helix::TextOutputStream::kColour_Red;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const char* Helix::GetOpcodeName(Opcode opcode)
{
	switch (opcode) {
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
	case kInsn_Ret:        return "ret";
	case kInsn_Br:         return "br";
	case kInsn_Cbr:        return "cbr";
	case kInsn_Call:       return "call";
	case kInsn_FCmp:       return "fcmp";
	case kInsn_ICmp:       return "icmp";

	case kInsn_Undefined:
	default:
		return "undef";
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const char* Helix::GetTypeName(Helix::TypeID id)
{
	switch (id) {
	case Helix::kType_Float32:      return "f32";
	case Helix::kType_Float64:      return "f64";
	case Helix::kType_Int8:         return "i8";
	case Helix::kType_Int16:        return "i16";
	case Helix::kType_Int32:        return "i32";
	case Helix::kType_Int64:        return "i64";
	case Helix::kType_LabelType:    return "label";
	case Helix::kType_FunctionType: return "function";
	case Helix::kType_Pointer:      return "ptr";
	case Helix::kType_Undefined:
	default:
		return "?";
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Helix::Print(TextOutputStream& out, const Value& value)
{
	if (const ConstantInt* ci = value_cast<ConstantInt>(&value)) {
		out.SetColour(kColour_Number); out.Write("%llu", ci->GetIntegralValue()); out.ResetColour();
	}
	else if (const VirtualRegisterName* vreg = value_cast<VirtualRegisterName>(&value)) {
		const char* name = vreg->GetDebugName();

		if (name) {
			out.Write("%%%s", name);
		} else {
			out.Write("%%%zu", vreg->GetSlot());
		}
	}

	const Type* typePtr  = value.GetType();
	const char* typeName = GetTypeName(typePtr->GetTypeID());

	out.Write(":");

	out.SetColour(kColour_Typename); out.Write("%s", typeName); out.ResetColour();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Helix::Print(TextOutputStream& out, const Instruction& insn)
{
	const char* opcodeName = Helix::GetOpcodeName(insn.GetOpcode());

	// Write out the instruction name/opcode (not technically a keyword??? but is
	// a reserved identifier so highlight it as a keyword.
	out.SetColour(kColour_Keyword); out.Write("%s", opcodeName); out.ResetColour();

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
	// Need to write out the word 'function' separately in order to be able to
	// highlight/colourise it properly.
	out.SetColour(kColour_Keyword); out.Write("function "); out.ResetColour();

	const std::string& name = fn.GetName();

	out.Write("%s() {\n", name.c_str());

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
		out.Write(".?");
	}

	out.Write(":\n");

	for (const Instruction& insn : bb) {
		out.Write("\t");
		Print(out, insn);
		out.Write("\n");
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
