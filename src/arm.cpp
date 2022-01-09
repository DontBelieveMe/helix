#include "mir.h"
#include "system.h"
#include "basic-block.h"

using namespace Helix;

//#pragma optimize("", off)

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

	// Load the address of the global into the destination register...
	BasicBlock::iterator where = LoadGlobalAddressIntoRegister(insn, load->GetDst(), load->GetSrc());

	// ... then load the value stored at the address in the destination register, into the destination register.
	// This seems like a bit of a hack that allows us to only use one register.
	//
	// #FIXME: Do a bit of an investigation, find out if this is legal (it seems to work?) or even just a bad idea.
	
	return ARMv7::CreateLdr(load->GetDst(), load->GetDst());
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
