#include "lower.h"
#include "system.h"
#include "function.h"
#include "instructions.h"

#include <vector>
#include <numeric>

#pragma optimize("", off)

using namespace Helix;

namespace ARMv7
{
	const Type* PointerType()
	{
		return BuiltinTypes::GetInt32();
	}

	size_t TypeSize(const Type* ty)
	{
		switch (ty->GetTypeID()) {
		case kType_Integer:
			return type_cast<IntegerType>(ty)->GetBitWidth() / 8;

		case kType_Array: {
			const ArrayType* arr = type_cast<ArrayType>(ty);
			return arr->GetCountElements() * ARMv7::TypeSize(arr->GetBaseType());
		}

		case kType_Pointer: {
			return 4;
		}

		case kType_Struct: {
			const StructType* st = type_cast<StructType>(ty);
			return std::accumulate(
				st->fields_begin(),
				st->fields_end(),
				0,
				[](size_t v, const Type* field) -> size_t {
					return v + ARMv7::TypeSize(field);
				}
			);
		}

		default:
			helix_unimplemented("TypeSize not implemented for type category");
		}
	}
}

namespace IR
{
	static void ReplaceAllUsesWith(Value* oldValue, Value* newValue)
	{
		std::vector<Use> worklist;

		for (auto it = oldValue->uses_begin(); it != oldValue->uses_end(); ++it) {
			worklist.push_back(*it);
		}

		for (Use& use : worklist) {
			use.ReplaceWith(newValue);
		}
	}
}

void GenericLowering::Lower_Lea(BasicBlock& bb, LoadEffectiveAddressInsn& insn)
{
	VirtualRegisterName* ptrint     = VirtualRegisterName::Create(ARMv7::PointerType());
	VirtualRegisterName* offset     = VirtualRegisterName::Create(ARMv7::PointerType());
	VirtualRegisterName* newAddress = VirtualRegisterName::Create(ARMv7::PointerType());
	VirtualRegisterName* intptr     = VirtualRegisterName::Create(BuiltinTypes::GetPointer());

	ConstantInt* typeSize = ConstantInt::Create(ARMv7::PointerType(), ARMv7::TypeSize(insn.GetBaseType()));

	BasicBlock::iterator where = bb.Where(&insn);
	
	where = bb.InsertAfter(where, Helix::CreatePtrToInt(ARMv7::PointerType(), insn.GetInputPtr(), ptrint));
	where = bb.InsertAfter(where, Helix::CreateBinOp(kInsn_IMul, insn.GetIndex(), typeSize, offset));
	where = bb.InsertAfter(where, Helix::CreateBinOp(kInsn_IAdd, ptrint, offset, newAddress));
	where = bb.InsertAfter(where, Helix::CreateIntToPtr(ARMv7::PointerType(), newAddress, intptr));

	IR::ReplaceAllUsesWith(insn.GetOutputPtr(), intptr);

	bb.Delete(bb.Where(&insn));
}

void GenericLowering::Lower_Lfa(BasicBlock& bb, LoadFieldAddressInsn& insn)
{
	VirtualRegisterName* inputInteger = VirtualRegisterName::Create(ARMv7::PointerType());
	VirtualRegisterName* newAddress = VirtualRegisterName::Create(ARMv7::PointerType());
	VirtualRegisterName* resultPointer = VirtualRegisterName::Create(BuiltinTypes::GetPointer());

	const StructType* structType = type_cast<StructType>(insn.GetBaseType());
	helix_assert(structType, "LoadFieldAddress should only ever have StructType base types");

	size_t offsetValue = 0;

	for (size_t i = 0; i < insn.GetFieldIndex(); ++i) {
		offsetValue += ARMv7::TypeSize(*(structType->fields_begin() + i));
	}

	ConstantInt* offset = ConstantInt::Create(ARMv7::PointerType(), offsetValue);

	BasicBlock::iterator where = bb.Where(&insn);

	where = bb.InsertAfter(where, Helix::CreatePtrToInt(ARMv7::PointerType(), insn.GetInputPtr(), inputInteger));
	where = bb.InsertAfter(where, Helix::CreateBinOp(kInsn_IAdd, inputInteger, offset, newAddress));
	where = bb.InsertAfter(where, Helix::CreateIntToPtr(ARMv7::PointerType(), newAddress, resultPointer));

	IR::ReplaceAllUsesWith(insn.GetOutputPtr(), resultPointer);

	bb.Delete(bb.Where(&insn));
}

void GenericLowering::Execute(Function* fn)
{
	struct WorkPair { Instruction* insn; BasicBlock* bb; };

	std::vector<WorkPair> worklist;

	for (BasicBlock& bb : fn->blocks()) {
		for (Instruction& insn : bb.insns()) {
			switch (insn.GetOpcode()) {
			case kInsn_LoadElementAddress:
			case kInsn_LoadFieldAddress:
				worklist.push_back({&insn, &bb});
				break;

			default:
				break;
			}
		}
	}

	for (const WorkPair& workload : worklist) {
		switch (workload.insn->GetOpcode()) {
		case kInsn_LoadElementAddress:
			this->Lower_Lea(*workload.bb, *static_cast<LoadEffectiveAddressInsn*>(workload.insn));
			break;

		case kInsn_LoadFieldAddress:
			this->Lower_Lfa(*workload.bb, *static_cast<LoadFieldAddressInsn*>(workload.insn));
			break;

		default:
			break;
		}
	}
}