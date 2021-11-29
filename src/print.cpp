#include "print.h"

using namespace Helix;

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
	case kInsn_FCmp_Eq:    return "fcmp_eq";
	case kInsn_FCmp_Neq:   return "fcmp_neq";
	case kInsn_FCmp_Lt:    return "fcmp_lt";
	case kInsn_FCmp_Gt:    return "fcmp_gt";
	case kInsn_FCmp_Lte:   return "fcmp_lte";
	case kInsn_FCmp_Gte:   return "fcmp_gte";
	case kInsn_ICmp_Eq:    return "icmp_eq";
	case kInsn_ICmp_Neq:   return "icmp_neq";
	case kInsn_ICmp_Lt:    return "icmp_lt";
	case kInsn_ICmp_Gt:    return "icmp_gt";
	case kInsn_ICmp_Lte:   return "icmp_lte";
	case kInsn_ICmp_Gte:   return "icmp_gte";

	case kInsn_Undefined:
	default:
		return "undef";
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const char* Helix::GetTypeName(const Helix::Type* type)
{
	switch (type->GetTypeID()) {
	case Helix::kType_Float32:      return "f32";
	case Helix::kType_Float64:      return "f64";
	case Helix::kType_LabelType:    return "label";
	case Helix::kType_FunctionType: return "function";
	case Helix::kType_Pointer:      return "ptr";
	case Helix::kType_Undefined:
	case Helix::kType_Integer: {
		const Helix::IntegerType* ty = Helix::type_cast<IntegerType>(type);

		switch (ty->GetBitWidth()) {
		case 8:  return "i8";
		case 16: return "i16";
		case 32: return "i32";
		case 64: return "i64";
		default:
			helix_unreachable("Unknown bit width");
			return "i?";
		}
	}
	default:
		return "?";
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void InternalPrint(SlotTracker& slots, TextOutputStream& out, const Value& value)
{
	bool isBranchTarget = false;

	if (const ConstantInt* ci = value_cast<ConstantInt>(&value)) {
		out.SetColour(kColour_Number); out.Write("%llu", ci->GetIntegralValue()); out.ResetColour();
	}
	else if (const VirtualRegisterName* vreg = value_cast<VirtualRegisterName>(&value)) {
		const char* name = vreg->GetDebugName();

		if (name) {
			out.Write("%%%s", name);
		} else {
			const size_t slot = slots.GetValueSlot(&value);
			out.Write("%%%zu", slot);
		}
	}
	else if(const BlockBranchTarget* bbt = value_cast<BlockBranchTarget>(&value)) {
		BasicBlock* bb = bbt->GetParent();
		out.Write(".%zu", slots.GetBasicBlockSlot(bb));
		isBranchTarget = true;
	}

	if (!isBranchTarget) {
		const Type* typePtr  = value.GetType();
		const char* typeName = GetTypeName(typePtr);

		out.Write(":");

		out.SetColour(kColour_Typename); out.Write("%s", typeName); out.ResetColour();
	}

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void InternalPrint(SlotTracker& slots, TextOutputStream& out, const Instruction& insn)
{
	const char* opcodeName = Helix::GetOpcodeName(insn.GetOpcode());

	// Write out the instruction name/opcode (not technically a keyword??? but is
	// a reserved identifier so highlight it as a keyword.
	out.SetColour(kColour_Keyword); out.Write("%s", opcodeName); out.ResetColour();

	const size_t nOperands = insn.GetCountOperands();

	if (nOperands > 0) {
		out.Write(" ");
	}

	if (insn.GetOpcode() == kInsn_StackAlloc) {
		const StackAllocInsn& stackAlloc = static_cast<const StackAllocInsn&>(insn);
		const char* typeName = Helix::GetTypeName(stackAlloc.GetType());
		const size_t count = stackAlloc.GetCount();

		out.Write("[%s x %zu], ", typeName, count);
	}

	for (size_t i = 0; i < nOperands; ++i) {
		Value* pValue = insn.GetOperand(i);

		if (pValue) {
			InternalPrint(slots, out, *pValue);
		}
		else {
			out.Write("?");
		}

		if (i < nOperands - 1) {
			out.Write(", ");
		}
	}

	if (Options::GetDebugAnnotateIR() && insn.HasComment()) {
		out.SetColour(TextOutputStream::kColour_Purple);
		const std::string& comment = insn.GetComment();
		out.Write("  # %s", comment.c_str());
		out.ResetColour();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void InternalPrint(SlotTracker& slots, TextOutputStream& out, const BasicBlock& bb)
{
	const char* name = bb.GetName();

	if (name) {
		out.Write(".%s", name);
	} else {
		out.Write(".%zu", slots.GetBasicBlockSlot(&bb));
	}

	out.Write(":");

	if (Options::GetDebugAnnotateIR() && bb.HasComment()) {
		const std::string& comment = bb.GetComment();
		out.SetColour(TextOutputStream::kColour_Purple);
		out.Write("  # %s", comment.c_str());
		out.ResetColour();
	}

	out.Write("\n");

	for (const Instruction& insn : bb) {
		out.Write("\t");
		InternalPrint(slots, out, insn);
		out.Write("\n");
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void InternalPrint(SlotTracker& slots, TextOutputStream& out, const Function& fn)
{
	// Need to write out the word 'function' separately in order to be able to
	// highlight/colourise it properly.
	out.SetColour(kColour_Keyword); out.Write("function "); out.ResetColour();

	const std::string& name = fn.GetName();

	out.Write("%s() {\n", name.c_str());

	for (const BasicBlock& bb : fn) {
		InternalPrint(slots, out, bb);
	}

	out.Write("}\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Helix::Print(TextOutputStream& out, const Value& value)
{
	SlotTracker slots;
	InternalPrint(slots, out, value);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Helix::Print(TextOutputStream& out, const Instruction& value)
{
	SlotTracker slots;
	InternalPrint(slots, out, value);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Helix::Print(TextOutputStream& out, const BasicBlock& value)
{
	SlotTracker slots;
	InternalPrint(slots, out, value);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Helix::Print(TextOutputStream& out, const Function& value)
{
	SlotTracker slots;
	InternalPrint(slots, out, value);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
