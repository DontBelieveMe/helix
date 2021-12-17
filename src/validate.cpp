/**
 * @file validate.cpp
 * @author Barney Wilks
 */

#include "validate.h"
#include "module.h"
#include "print.h"

using namespace Helix;

#define CASE_CHECK_INSN(Opcode, ClassName)            \
	case Opcode: {                                    \
		ClassName& t = static_cast<ClassName&>(insn); \
		error |= !(Check##ClassName(fn, bb, t));      \
		break;                                        \
	}

bool CheckRetInsn(Function* fn, BasicBlock&, RetInsn& insn) {
	if (insn.HasReturnValue() && fn->IsVoidReturn()) {
		helix_error(logs::validate, "invalid return, that returns a value from a void function");
		return false;
	}

	if (!insn.HasReturnValue() && !fn->IsVoidReturn()) {
		helix_error(logs::validate, "invalid return, should return value from non void function");
		return false;
	}

	if (insn.HasReturnValue()) {
		if (fn->GetReturnType() != insn.GetReturnValue()->GetType()) {
			helix_error(logs::validate, "invalid return, mismatch between return value type and function return type");
			return false;
		}
	}

	return true;
}

bool CheckBinOpInsn(Function*, BasicBlock&, BinOpInsn& insn) {
	if (insn.GetLHS()->GetType() != insn.GetRHS()->GetType()) {
		helix_error(logs::validate, "invalid binop, lhs and rhs should have the same type");
		return false;
	}

	return true;
}

bool CheckStackAllocInsn(Function*, BasicBlock&, StackAllocInsn& insn) {
	if (!insn.GetOutputPtr()->GetType()->IsPointer()) {
		helix_error(logs::validate, "invalid stack_alloc, output value should have pointer type");
		return false;
	}

	return true;
}

bool CheckStoreInsn(Function*, BasicBlock&, StoreInsn& store) {
	if (!store.GetDst()->GetType()->IsPointer()) {
        helix_error(logs::validate, "invalid store, destination value should have pointer type (store value -> memory)");
		return false;
    }

	return true;
}

bool CheckLoadInsn(Function*, BasicBlock&, LoadInsn& load) {
    if (!load.GetSrc()->GetType()->IsPointer()) {
        helix_error(logs::validate, "invalid load, source value should have pointer type (load from memory -> value)");
        return false;
    }

    return true;
}

bool CheckUnconditionalBranchInsn(Function*, BasicBlock&, UnconditionalBranchInsn&) {
    return true;
}

bool CheckConditionalBranchInsn(Function*, BasicBlock&, ConditionalBranchInsn& br) {
    // #FIXME: not technically an error (more a warning?) but definately fishy
    if (br.GetTrueBB() == br.GetFalseBB()) {
        helix_error(logs::validate, "invalid conditional branch, true bb and false bb are the same");
        return false;        
    }

    // #FIXME: should it?? maybe we should add a dedicated boolean type?
    if (!br.GetCond()->GetType()->IsIntegral()) {
        helix_error(logs::validate, "invalid conditional branch, condition type should be integral");
        return false;
    }

    return true;
}

bool CheckCompareInsn(Function*, BasicBlock&, CompareInsn&) {
    return true;
}

bool CheckCastInsn(Function*,BasicBlock&,CastInsn& cast) {
	if (cast.GetOpcode() == Helix::kInsn_PtrToInt) {
		if (!cast.GetSrcType()->IsPointer()) {
			helix_error(logs::validate, "bad ptrtoint, source type is not a pointer");
			return false;
		}

		if (!cast.GetDstType()->IsIntegral()) {
			helix_error(logs::validate, "bad ptrtoint, destination type is not an integer");
			return false;
		}
	}
	else if (cast.GetOpcode() == Helix::kInsn_IntToPtr) {
		if (!cast.GetSrcType()->IsIntegral()) {
			helix_error(logs::validate, "bad inttoptr, source type is not an integer");
			return false;
		}

		if (!cast.GetDstType()->IsPointer()) {
			helix_error(logs::validate, "bad inttoptr, destination type is not a pointer");
			return false;
		}
	}

	return true;
}

void ValidationPass::Execute(Module* module) {
	bool error = false;

	for (Function* fn : module->functions()) {
		const std::string& name = fn->GetName();

		if (name.empty()) {
			helix_error(logs::validate, "Function has an empty name, must be a valid symbol");
			error |= true;
		}

		if (fn->GetCountBlocks() == 0) {
			helix_error(logs::validate, "Function '{}' has no basic blocks, must have at least one", name);
			error |= true;
		}

		for (BasicBlock& bb : fn->blocks()) {
			if (!bb.HasTerminator()) {
				helix_error(logs::validate, "Basic block in function '{}' does not finish with a terminator insn", name);
				error |= true;
			}

			for (Instruction& insn : bb.insns()) {
				switch (insn.GetOpcode()) {
				CASE_CHECK_INSN(kInsn_Return, RetInsn);
				CASE_CHECK_INSN(kInsn_IAdd, BinOpInsn);
				CASE_CHECK_INSN(kInsn_ISub, BinOpInsn);
				CASE_CHECK_INSN(kInsn_IDiv, BinOpInsn);
				CASE_CHECK_INSN(kInsn_IMul, BinOpInsn);
				CASE_CHECK_INSN(kInsn_IRem, BinOpInsn);
				CASE_CHECK_INSN(kInsn_And, BinOpInsn);
				CASE_CHECK_INSN(kInsn_Or, BinOpInsn);
				CASE_CHECK_INSN(kInsn_Xor, BinOpInsn);
				CASE_CHECK_INSN(kInsn_Shl, BinOpInsn);
				CASE_CHECK_INSN(kInsn_Shr, BinOpInsn);
				CASE_CHECK_INSN(kInsn_StackAlloc, StackAllocInsn);
				CASE_CHECK_INSN(kInsn_Store, StoreInsn);
				CASE_CHECK_INSN(kInsn_Load, LoadInsn);
				CASE_CHECK_INSN(kInsn_ConditionalBranch, ConditionalBranchInsn);
				CASE_CHECK_INSN(kInsn_UnconditionalBranch, UnconditionalBranchInsn);
				CASE_CHECK_INSN(kInsn_ICmp_Eq, CompareInsn);
				CASE_CHECK_INSN(kInsn_ICmp_Neq, CompareInsn);
				CASE_CHECK_INSN(kInsn_ICmp_Gt, CompareInsn);
				CASE_CHECK_INSN(kInsn_ICmp_Lt, CompareInsn);
				CASE_CHECK_INSN(kInsn_ICmp_Gte, CompareInsn);
				CASE_CHECK_INSN(kInsn_ICmp_Lte, CompareInsn);
				CASE_CHECK_INSN(kInsn_IntToPtr, CastInsn);
				CASE_CHECK_INSN(kInsn_PtrToInt, CastInsn);
				default: {
					helix_warn(logs::validate, "instruction '{}' has no validation rules, ignoring", GetOpcodeName(insn.GetOpcode()));
					break;
				}
				}
			}
		}
	}

	helix_assert(!error, "Module failed validation, check 'validate' log check (--log=validate)");
}