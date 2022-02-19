#include "genlower.h"
#include "basic-block.h"
#include "ir-helpers.h"
#include "function.h"

using namespace Helix;

/*********************************************************************************************************************/

void GenericLowering::LowerIRem(BasicBlock& bb, BinOpInsn& insn)
{
	// #FIXME: This is really an ARM specific optimisation? Can take advantage of a `mls`
	//         instruction there. Move out of genlower into a different armlower pass.

	// a % b = a - ((a / b) * b)

	Value* lhs = insn.GetLHS();
	Value* rhs = insn.GetRHS();
	Value* dst = insn.GetResult();

	const Type* operandType = insn.GetLHS()->GetType();

	VirtualRegisterName* t0 = VirtualRegisterName::Create(operandType);
	VirtualRegisterName* t1 = VirtualRegisterName::Create(operandType);

	BasicBlock::iterator where = bb.Where(&insn);

	const HLIR::Opcode divop
		= insn.GetOpcode() == HLIR::ISRem ? HLIR::ISDiv : HLIR::IUDiv;

	where = bb.InsertAfter(where, Helix::CreateBinOp(divop, lhs, rhs, t0));
	where = bb.InsertAfter(where, Helix::CreateBinOp(HLIR::IMul, t0, rhs, t1));
	where = bb.InsertAfter(where, Helix::CreateBinOp(HLIR::ISub, lhs, t1, dst));

	insn.DeleteFromParent();
}

/*********************************************************************************************************************/

void GenericLowering::LowerLea(BasicBlock& bb, LoadEffectiveAddressInsn& insn)
{
	VirtualRegisterName* ptrint     = VirtualRegisterName::Create(ARMv7::PointerType());
	VirtualRegisterName* offset     = VirtualRegisterName::Create(ARMv7::PointerType());
	VirtualRegisterName* newAddress = VirtualRegisterName::Create(ARMv7::PointerType());
	VirtualRegisterName* intptr     = VirtualRegisterName::Create(BuiltinTypes::GetPointer());

	ConstantInt* typeSize = ConstantInt::Create(ARMv7::PointerType(), ARMv7::TypeSize(insn.GetBaseType()));

	BasicBlock::iterator where = bb.Where(&insn);
	
	where = bb.InsertAfter(where, Helix::CreatePtrToInt(insn.GetInputPtr(), ptrint));
	where = bb.InsertAfter(where, Helix::CreateBinOp(HLIR::IMul, insn.GetIndex(), typeSize, offset));
	where = bb.InsertAfter(where, Helix::CreateBinOp(HLIR::IAdd, ptrint, offset, newAddress));
	where = bb.InsertAfter(where, Helix::CreateIntToPtr(newAddress, intptr));

	IR::ReplaceAllUsesWith(insn.GetOutputPtr(), intptr);

	insn.DeleteFromParent();
}

/*********************************************************************************************************************/

void GenericLowering::LowerLfa(BasicBlock& bb, LoadFieldAddressInsn& insn)
{
	VirtualRegisterName* inputInteger = VirtualRegisterName::Create(ARMv7::PointerType());
	VirtualRegisterName* newAddress = VirtualRegisterName::Create(ARMv7::PointerType());
	VirtualRegisterName* resultPointer = VirtualRegisterName::Create(BuiltinTypes::GetPointer());

	const StructType* structType = type_cast<StructType>(insn.GetBaseType());
	helix_assert(structType, "LoadFieldAddress should only ever have StructType base types");

	size_t offsetValue = 0;

	for (size_t i = 0; i < insn.GetFieldIndex(); ++i) {
		offsetValue += ARMv7::TypeSize(structType->GetField(i));
	}

	ConstantInt* offset = ConstantInt::Create(ARMv7::PointerType(), offsetValue);

	BasicBlock::iterator where = bb.Where(&insn);

	where = bb.InsertAfter(where, Helix::CreatePtrToInt(insn.GetInputPtr(), inputInteger));
	where = bb.InsertAfter(where, Helix::CreateBinOp(HLIR::IAdd, inputInteger, offset, newAddress));
	where = bb.InsertAfter(where, Helix::CreateIntToPtr(newAddress, resultPointer));

	IR::ReplaceAllUsesWith(insn.GetOutputPtr(), resultPointer);

	insn.DeleteFromParent();
}

/*********************************************************************************************************************/

void GenericLowering::Execute(Function* fn, const PassRunInformation&)
{
	struct WorkPair { Instruction* insn; BasicBlock* bb; };

	std::vector<WorkPair> worklist;

	for (BasicBlock& bb : fn->blocks()) {
		for (Instruction& insn : bb.insns()) {
			switch (insn.GetOpcode()) {
			case HLIR::LoadElementAddress:
			case HLIR::LoadFieldAddress:
			case HLIR::IURem:
			case HLIR::ISRem:
				worklist.push_back({&insn, &bb});
				break;

			default:
				break;
			}
		}
	}

	helix_debug(logs::genlower, "Found {0} instructions that require lowering", worklist.size());

	for (const WorkPair& workload : worklist) {
		switch (workload.insn->GetOpcode()) {
		case HLIR::LoadElementAddress:
			this->LowerLea(*workload.bb, *static_cast<LoadEffectiveAddressInsn*>(workload.insn));
			break;

		case HLIR::LoadFieldAddress:
			this->LowerLfa(*workload.bb, *static_cast<LoadFieldAddressInsn*>(workload.insn));
			break;

		case HLIR::ISRem:
		case HLIR::IURem:
			this->LowerIRem(*workload.bb, *static_cast<BinOpInsn*>(workload.insn));
			break;

		default:
			break;
		}
	}
}

/*********************************************************************************************************************/
