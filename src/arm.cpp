#include "mir.h"
#include "system.h"
#include "basic-block.h"
#include "ir-helpers.h"

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

static MachineInstruction* CreateMachineLoad(Value* dst, Value* src, MachineMode mode, bool signExtend)
{
	switch (mode) {
	case QImode: return signExtend ? ARMv7::CreateLdrsb(dst, src) : ARMv7::CreateLdrb(dst, src);
	case HImode: return signExtend ? ARMv7::CreateLdrsh(dst, src) : ARMv7::CreateLdrh(dst, src);
	case SImode: return ARMv7::CreateLdr(dst, src);
	default:
		break;
	}

	helix_unreachable("unacceptable machine mode for load");
	return nullptr;
}

/*********************************************************************************************************************/

MachineInstruction* ARMv7::expand_load(Instruction* insn)
{
	helix_assert(insn->GetOpcode() == HLIR::Load, "cannot expand load instruction that doesn't have kInsn_Load opcode");

	LoadInsn* load = (LoadInsn*) insn;

	Value* loadDestination = load->GetDst();
	Value* loadSource      = load->GetSrc();

	const MachineMode destinationMachineMode = GetMachineMode(loadDestination);

	bool bSignExtend = false;

	Use use;
	if (IR::TryGetSingleUser(load, loadDestination, &use)) {
		// If we have a single user of the output of the load
		// check if it's a zero/sign extension instruction.
		//
		// If it's a sign extension then we need to emit a
		// special LDRS* instruction that does a load and
		// sign extend.
		//
		// If it's a zero extension then we don't need
		// to do anything since LDR* does zero extension
		// by default.
		//
		// Either way, we need to remove the zext/sext
		// instruction & update any references/use to reflect
		// such

		Instruction* singleUser = use.GetInstruction();

		switch (singleUser->GetOpcode()) {
		case HLIR::SExt:
			bSignExtend = true;

		// Can just fallthrough here, since it's the same logic for handling
		// the zext & sext instructions themselves.
		[[fallthrough]];
		case HLIR::ZExt: {
			CastInsn* castInstruction = static_cast<CastInsn*>(singleUser);

			helix_assert(castInstruction->GetSrc() == loadDestination, "Load/Cast destination mismatch");

			// First replace any uses of the sext/zext result with
			// the load destination value (we're basically 'forwarding'
			// through the cast & acting like it never existed)...
			IR::ReplaceAllUsesWith(castInstruction->GetDst(), loadDestination);

			// ... and finally remove the instruction itself
			castInstruction->DeleteFromParent();

			helix_assert(ARMv7::TypeSize(loadDestination->GetType()) <= 4, "Load destination can't fit into a single register");

			loadDestination->SetType(BuiltinTypes::GetInt32());

			break;
		}

		default:
			break;
		}
	}

	if (is_register(loadSource) && is_register(loadDestination)) {
		return CreateMachineLoad(loadDestination, loadSource, destinationMachineMode, bSignExtend);
	}
	else if (is_global(loadSource) && is_register(loadDestination)) {
		// Load the address of the global into this temporary 'addressRegister'
		VirtualRegisterName* addressRegister = VirtualRegisterName::Create(BuiltinTypes::GetPointer());
		LoadGlobalAddressIntoRegister(insn, addressRegister, loadSource);

		return CreateMachineLoad(loadDestination, addressRegister, destinationMachineMode, bSignExtend);
	}

	helix_unreachable("Unexpected types for source & destination operands in load");

	return nullptr;
}

/*********************************************************************************************************************/

static MachineInstruction* expand_icmp_branch_pair(CompareInsn* compare, ConditionalBranchInsn* branch)
{
	helix_assert(compare->GetParent() == branch->GetParent(), "Cannot expand icmp/branch pair with different parent blocks");

	Value* trueTarget = branch->GetTrueTarget();
	Value* falseTarget = branch->GetFalseTarget();

	MachineInstruction* br = nullptr;
	switch (compare->GetOpcode()) {
	case HLIR::ICmp_Eq:  br = ARMv7::CreateBeq(trueTarget); break;
	case HLIR::ICmp_Neq: br = ARMv7::CreateBne(trueTarget); break;
	case HLIR::ICmp_Gt:  br = ARMv7::CreateBgt(trueTarget); break;
	case HLIR::ICmp_Gte: br = ARMv7::CreateBge(trueTarget); break;
	case HLIR::ICmp_Lt:  br = ARMv7::CreateBlt(trueTarget); break;
	case HLIR::ICmp_Lte: br = ARMv7::CreateBle(trueTarget); break;
	default:
		helix_unreachable("Unknown comparison type (compare/branch expansion)");
	}

	branch->DeleteFromParent(); branch = nullptr;

	BasicBlock* bb = compare->GetParent();
	BasicBlock::iterator where = bb->Where(compare);

	where = bb->InsertBefore(where, ARMv7::CreateCmp(compare->GetLHS(), compare->GetRHS()));
	where = bb->InsertAfter(where, br);

	return ARMv7::CreateBr(falseTarget);
}

/*********************************************************************************************************************/

MachineInstruction* ARMv7::expand_icmp(Instruction* insn)
{
	helix_assert(!Helix::IsMachineOpcode(insn->GetOpcode()), "Can't expand LLIR instruction");
	helix_assert(HLIR::IsCompare((HLIR::Opcode) insn->GetOpcode()), "instruction is not a comparison");

	CompareInsn* compare = (CompareInsn*) insn;

	Use use;
	if (IR::TryGetSingleUser(insn, compare->GetResult(), &use)) {
		if (use.GetInstruction()->GetOpcode() == HLIR::ConditionalBranch) {
			return expand_icmp_branch_pair(compare, (ConditionalBranchInsn*) use.GetInstruction());
		}
	}

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
	case HLIR::ICmp_Eq:  cmov = ARMv7::CreateMovweqi(result, one); break;
	case HLIR::ICmp_Neq: cmov = ARMv7::CreateMovwnei(result, one); break;
	case HLIR::ICmp_Gt:  cmov = ARMv7::CreateMovwgti(result, one); break;
	case HLIR::ICmp_Gte: cmov = ARMv7::CreateMovwgei(result, one); break;
	case HLIR::ICmp_Lt:  cmov = ARMv7::CreateMovwlti(result, one); break;
	case HLIR::ICmp_Lte: cmov = ARMv7::CreateMovwlei(result, one); break;
	default:
		helix_unreachable("unknown comparison opcode");
		break;
	}

	return cmov;
}

/*********************************************************************************************************************/

MachineInstruction* ARMv7::expand_conditional_branch(Instruction* insn)
{
	helix_assert(insn->GetOpcode() == HLIR::ConditionalBranch, "instruction is not a conditional branch");

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
	helix_assert(insn->GetOpcode() == HLIR::Store, "instruction is not a store");
	StoreInsn* store = (StoreInsn*) insn;

	// If the the source is a global variable then store the address to the memory
	// address given in dst.
	if (value_isa<GlobalVariable>(store->GetSrc())) {
		VirtualRegisterName* globalAddress = VirtualRegisterName::Create(BuiltinTypes::GetInt32());
		LoadGlobalAddressIntoRegister(store, globalAddress, store->GetSrc());
		return ARMv7::CreateStr(globalAddress, store->GetDst());
	}
	else if (value_isa<GlobalVariable>(store->GetDst())) {
		VirtualRegisterName* globalAddress = VirtualRegisterName::Create(BuiltinTypes::GetInt32());

		// #FIXME: Support storing types other than i32 (i16 & i8 primarily, i64 support can wait. don't even mention fp).
		helix_assert(GetMachineMode(store->GetSrc()) == SImode, "unexpected machine mode for store (currently unsupported)");

		LoadGlobalAddressIntoRegister(store, globalAddress, store->GetDst());
		return ARMv7::CreateStr(store->GetSrc(), globalAddress);
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
	helix_assert(insn->GetOpcode() == HLIR::PtrToInt, "instruction is not ptrtoint");
	CastInsn* castInsn = (CastInsn*) insn;
	helix_assert(is_global(castInsn->GetSrc()), "ptrtoint source must be a global for this transform");

	BasicBlock::iterator where = LoadGlobalAddressIntoRegister(insn, castInsn->GetDst(), castInsn->GetSrc());
	helix_assert(Helix::IsMachineOpcode(where->GetOpcode()), "insn is not a machine instruction");
	return (MachineInstruction*) &(*where);
}

/*********************************************************************************************************************/

bool Helix::IsMachineTerminator(OpcodeType opc)
{
	switch (opc) {
	case ARMv7::Bge:
	case ARMv7::Bgt:
	case ARMv7::Blt:
	case ARMv7::Ble:
	case ARMv7::Beq:
	case ARMv7::Bne:
	case ARMv7::Br:
	case ARMv7::Ret:
		return true;

	default:
		return false;
	}
}

/*********************************************************************************************************************/
