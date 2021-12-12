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
		offsetValue += ARMv7::TypeSize(structType->GetField(i));
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

void GenericLegalizer::LegaliseStore(BasicBlock& bb, StoreInsn& store)
{
	helix_assert(store.GetDst()->GetType()->IsPointer(), "must store into a pointer");

	// The type we're storing into the memory address given by dst
	const Type* storeType = store.GetSrc()->GetType();

	Value* src = store.GetSrc();
	Value* dst = store.GetDst();

	if (const ArrayType* srcArrayType = type_cast<ArrayType>(storeType)) {
		if (ConstantArray* constantArray = value_cast<ConstantArray>(src)) {
			BasicBlock::iterator where = bb.Where(&store);

			for (size_t i = 0; i < constantArray->GetCountValues(); ++i) {
				Value* init = constantArray->GetValue(i);

				VirtualRegisterName* ptr = VirtualRegisterName::Create(BuiltinTypes::GetPointer());
				ConstantInt* index = ConstantInt::Create(BuiltinTypes::GetInt32(), i);
				where = bb.InsertAfter(where, Helix::CreateLoadEffectiveAddress(srcArrayType->GetBaseType(), dst, index, ptr));
				where = bb.InsertAfter(where, Helix::CreateStore(init, ptr));
			}

			bb.Delete(bb.Where(&store));
		}

		return;
	}
	else if (const StructType* srcStructType = type_cast<StructType>(storeType)) {
		if (ConstantStruct* constantStruct = value_cast<ConstantStruct>(src)) {
			BasicBlock::iterator where = bb.Where(&store);

			for (size_t i = 0; i < constantStruct->GetCountValues(); ++i) {
				Value* initValue = constantStruct->GetValue(i);
				VirtualRegisterName* ptr = VirtualRegisterName::Create(BuiltinTypes::GetPointer());
				
				where = bb.InsertAfter(where, Helix::CreateLoadFieldAddress(srcStructType, dst, i, ptr));
				where = bb.InsertAfter(where, Helix::CreateStore(initValue, ptr));
			}

			bb.Delete(bb.Where(&store));
		}

		return;
	}

	helix_unreachable("cannot legalise store with unsupported value type");
}

void GenericLegalizer::Execute(Function* fn)
{
	struct Store { StoreInsn& insn; BasicBlock& bb; };

	bool dirty = true;

	do {
		std::vector<Store> illegalStores;

		for (BasicBlock& bb : fn->blocks()) {
			for (Instruction& insn : bb.insns()) {
				switch (insn.GetOpcode()) {
				case kInsn_Store: {
					StoreInsn& store = static_cast<StoreInsn&>(insn);

					if (value_isa<ConstantArray>(store.GetSrc()) || value_isa<ConstantStruct>(store.GetSrc())) {
						illegalStores.push_back({store, bb });
					}

					break;
				}

				default:
					break;
				}
			}
		}

		for (Store& store : illegalStores) {
			LegaliseStore(store.bb, store.insn);
		}

		dirty = !illegalStores.empty();
	} while (dirty);
}