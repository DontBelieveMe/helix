#include "mir.h"
#include "system.h"
#include "basic-block.h"

using namespace Helix;

//#pragma optimize("", off)

/*********************************************************************************************************************/

enum MachineMode
{
	QImode,
	HImode,
	SImode,
	DImode,
	
	UndefinedMode
};

static MachineMode GetMachineMode(const Type* type)
{
	if (type->IsPointer()) {
		return SImode;
	}

	if (const IntegerType* int_type = type_cast<IntegerType>(type)) {
		switch (int_type->GetBitWidth()) {
		case 8:  return QImode;
		case 16: return HImode;
		case 32: return SImode;
		case 64: return DImode;
		}
	}

	helix_unreachable("no machine mode for type");
	return UndefinedMode;
}

/*********************************************************************************************************************/

static MachineMode GetMachineMode(Value* value)
{
	return GetMachineMode(value->GetType());
}

/*********************************************************************************************************************/

static BasicBlock::iterator LoadGlobalAddressIntoRegister(Instruction* insn, Value* dest_register, Value* source_global)
{
	BasicBlock* bb = insn->GetParent();
	BasicBlock::iterator where = bb->Where(insn);

	// Load the lower 16 bits of the address into the lower 16 bits of the register.
	where = bb->InsertBefore(where, ARMv7::CreateMovw_gl16(dest_register, source_global));

	// Load the upper 16 bits of the address into the upper 16 bits of the register.
	where = bb->InsertAfter(where,  ARMv7::CreateMovt_gu16(dest_register, source_global));

	return where;
}

/*********************************************************************************************************************/

MachineInstruction* ARMv7::expand_load(Instruction* insn)
{
	helix_assert(insn->GetOpcode() == kInsn_Load, "cannot expand load instruction that doesn't have kInsn_Load opcode");

	LoadInsn* load = (LoadInsn*) insn;

	if (is_register(load->GetSrc()) && is_register(load->GetDst())) {
		switch (GetMachineMode(load->GetDst())) {
		case QImode: return ARMv7::CreateLdrb(load->GetDst(), load->GetSrc());
		case HImode: return ARMv7::CreateLdrh(load->GetDst(), load->GetSrc());
		case SImode: return ARMv7::CreateLdr(load->GetDst(), load->GetSrc());
		default:
			helix_unreachable("unacceptable machine mode for load value (from mem reg)");
			break;
		}
	}
	else if (is_global(load->GetSrc()) && is_register(load->GetDst())) {
		// Load the address of the global into the destination register...
		BasicBlock::iterator where = LoadGlobalAddressIntoRegister(insn, load->GetDst(), load->GetSrc());

		// ... then load the value stored at the address in the destination register, into the destination register.
		// This seems like a bit of a hack that allows us to only use one register.
		//
		// #FIXME: Do a bit of an investigation, find out if this is legal (it seems to work?) or even just a bad idea.
		
		switch (GetMachineMode(load->GetDst())) {
		case QImode: return ARMv7::CreateLdrb(load->GetDst(), load->GetDst());
		case HImode: return ARMv7::CreateLdrh(load->GetDst(), load->GetDst());
		case SImode: return ARMv7::CreateLdr(load->GetDst(), load->GetDst());
		default:
			helix_unreachable("cannot natively load values of this machine mode (from global)");
			break;
		}
	}

	return nullptr;
}

/*********************************************************************************************************************/

MachineInstruction* ARMv7::expand_icmp(Instruction* insn)
{
	helix_assert(Helix::IsCompare(insn->GetOpcode()), "instruction is not a comparison");

	CompareInsn* compare = (CompareInsn*) insn;

	BasicBlock*          bb    = compare->GetParent();
	BasicBlock::iterator where = bb->Where(compare);

	ConstantInt* zero = ConstantInt::GetZero(BuiltinTypes::GetInt32());
	ConstantInt* one  = ConstantInt::GetOne(BuiltinTypes::GetInt32());

	Value* result = compare->GetResult();

	// icmp_* operations are currently synthesized as the following assembly
	//
	//  cmp   lhs, rhs     ; Compare the values, update the flags
	//  mov   result, #0   ; Zero the result register
	//  movw* result, #1   ; Conditionally move #1 (true) into the bottom half of
	//                       the result register if the comparison was true (based on flags).
	//
	// This is not (by far!) the best way to do this all (of even some?) of the time,
	// but it is simple to emit and works for now, so thats good enough :-)

	// Compare the LHS and RHS of the icmp, updating the flags...
	where = bb->InsertBefore(where, ARMv7::CreateCmp(compare->GetLHS(), compare->GetRHS()));

	// ... zero the result register (so that if the condition is false it will contain 'false/0')...
	where = bb->InsertAfter(where, ARMv7::CreateMovi(result, zero));

	MachineInstruction* cmov = nullptr;

	// ... finally generate the conditional move that sets the result register to true (1)
	// if the comparison was true (based on flags from cmp operation before.)
	switch (compare->GetOpcode()) {
	case kInsn_ICmp_Eq:  cmov = ARMv7::CreateMovweqi(result, one); break;
	case kInsn_ICmp_Neq: cmov = ARMv7::CreateMovwnei(result, one); break;
	case kInsn_ICmp_Gt:  cmov = ARMv7::CreateMovwgti(result, one); break;
	case kInsn_ICmp_Gte: cmov = ARMv7::CreateMovwgei(result, one); break;
	case kInsn_ICmp_Lt:  cmov = ARMv7::CreateMovwlti(result, one); break;
	case kInsn_ICmp_Lte: cmov = ARMv7::CreateMovwlei(result, one); break;
	default:
		helix_unreachable("unknown comparison opcode");
		break;
	}

	return cmov;
}

/*********************************************************************************************************************/

MachineInstruction* ARMv7::expand_conditional_branch(Instruction* insn)
{
	helix_assert(insn->GetOpcode() == kInsn_ConditionalBranch, "instruction is not a conditional branch");

	ConditionalBranchInsn* conditional_branch = (ConditionalBranchInsn*) insn;

	BasicBlock*          bb    = conditional_branch->GetParent();
	BasicBlock::iterator where = bb->Where(conditional_branch);

	ConstantInt* one  = ConstantInt::GetOne(BuiltinTypes::GetInt32());

	// Similar to icmp*, conditional branches get synthesized into the following assembly:
	//
	//  cmp condition, #1    ; Compare the condition with 1 & set flags (aka is the condition true?)
	//  bge true_bb          ; Conditionally branch to the true BB if the condition held (was true, from flags)
	//  b false_bb           ; Otherwise (execution didn't branch to the true target), so the condition must be false.
	//                       ; Jump to the false BB target.
	//
	// Again, this isn't a great way to do this, but it works & is consistent/easy to emit so... :-)
	// #FIXME: Look into improving this.

	// Emit the comparison, to setting the flags based on if the condition is true (equals 1)...
	where = bb->InsertBefore(where, ARMv7::CreateCmpi(conditional_branch->GetCond(), one));

	// ... if the condition was true (based on the cmp above), branch to the true target...
	where = bb->InsertAfter(where, ARMv7::CreateBge(conditional_branch->GetTrueTarget()));

	// ... if control flow gets here then it must mean the condition was false, since it didn't branch
	// to the true target, so instead branch to the false BB target.
	return ARMv7::CreateBr(conditional_branch->GetFalseTarget());
}

/*********************************************************************************************************************/

MachineInstruction* ARMv7::expand_store(Instruction* insn)
{
	helix_assert(insn->GetOpcode() == kInsn_Store, "instruction is not a store");
	StoreInsn* store = (StoreInsn*) insn;

	// Use r7 as a temporary/scratch register to store the address of the global
	// so we can store the value from that temp register to memory.
	// This is currently reserved by the register allocator for this purpose, but it is
	// quite a shitty thing to be doing, every little help counts when it comes to register allocation.

	PhysicalRegisterName* r7 = PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::R7);

	// If the the source is a global variable then store the address to the memory
	// address given in dst.
	if (value_isa<GlobalVariable>(store->GetSrc())) {
		LoadGlobalAddressIntoRegister(store, r7, store->GetSrc());
		return ARMv7::CreateStr(r7, store->GetDst());
	}
	else if (value_isa<GlobalVariable>(store->GetDst())) {
		// #FIXME: Support storing types other than i32 (i16 & i8 primarily, i64 support can wait. don't even mention fp).
		helix_assert(GetMachineMode(store->GetSrc()) == SImode, "unexpected machine mode for store (currently unsupported)");

		LoadGlobalAddressIntoRegister(store, r7, store->GetDst());
		return ARMv7::CreateStr(store->GetSrc(), r7);
	}
	else if (is_register(store->GetSrc()) && is_register(store->GetDst())) {
		switch (GetMachineMode(store->GetSrc())) {
		case QImode: return ARMv7::CreateStrb(store->GetSrc(), store->GetDst());
		case HImode: return ARMv7::CreateStrh(store->GetSrc(), store->GetDst());
		case SImode: return ARMv7::CreateStr(store->GetSrc(), store->GetDst());
		default:
			helix_unreachable("unacceptable machine mode for store soure value");
			break;
		}
	}

	helix_unreachable("cannot expand this form of store");
	return nullptr;
}

/*********************************************************************************************************************/

MachineInstruction* ARMv7::expand_global_address_to_register(Instruction* insn)
{
	helix_assert(insn->GetOpcode() == kInsn_PtrToInt, "instruction is not ptrtoint");
	CastInsn* castInsn = (CastInsn*) insn;
	helix_assert(is_global(castInsn->GetSrc()), "ptrtoint source must be a global for this transform");

	BasicBlock::iterator where = LoadGlobalAddressIntoRegister(insn, castInsn->GetDst(), castInsn->GetSrc());
	helix_assert(Helix::IsMachineOpcode(where->GetOpcode()), "insn is not a machine instruction");
	return (MachineInstruction*) &(*where);
}

/*********************************************************************************************************************/