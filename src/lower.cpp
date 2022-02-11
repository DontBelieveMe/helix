#include "lower.h"
#include "system.h"
#include "function.h"
#include "instructions.h"
#include "target-info-armv7.h"
#include "module.h"
#include "print.h"
#include "ir-helpers.h"

#include "arm-md.h" /* generated */

#include <vector>

// #pragma optimize("", off)

using namespace Helix;

/*********************************************************************************************************************/

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

			for (unsigned i = 0; i < (unsigned) constantStruct->GetCountValues(); ++i) {
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

/*********************************************************************************************************************/

void GenericLegalizer::Execute(Function* fn, const PassRunInformation&)
{
	helix_assert(fn->GetCountBlocks() > 0, "function must have at least one basic block");

	struct Store { StoreInsn& insn; BasicBlock& bb; };
	struct StackAlloc { StackAllocInsn& insn; BasicBlock& bb; };

	bool dirty = true;

	do {
		std::vector<Store> illegalStores;
		std::vector<StackAlloc> illegalStackAllocs;

		for (BasicBlock& bb : fn->blocks()) {
			for (Instruction& insn : bb.insns()) {
				switch (insn.GetOpcode()) {
				case HLIR::Store: {
					StoreInsn& store = static_cast<StoreInsn&>(insn);

					if (value_isa<ConstantArray>(store.GetSrc()) || value_isa<ConstantStruct>(store.GetSrc())) {
						illegalStores.push_back({store, bb });
					}

					break;
				}
				case HLIR::StackAlloc: {
					// Don't want to move stack_allocs that are already in the
					// first basic block of the function 
					if (fn->Where(&bb) == fn->begin()) {
						break;
					}

					StackAllocInsn& stack_alloc = static_cast<StackAllocInsn&>(insn);
					illegalStackAllocs.push_back({stack_alloc, bb});
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

		BasicBlock& head = *fn->begin();

		for (StackAlloc& stack_alloc : illegalStackAllocs) {
			BasicBlock& bb = stack_alloc.bb;

			bb.Remove(bb.Where(&stack_alloc.insn));
			head.InsertBefore(head.begin(), &stack_alloc.insn);
		}

		dirty = illegalStores.size() > 0 || illegalStackAllocs.size() > 0;
	} while (dirty);
}

/*********************************************************************************************************************/

void ReturnCombine::Execute(Function* fn, const PassRunInformation&)
{
	helix_assert(fn->GetCountBlocks() >= 1, "Function must have at least one basic block");

	// Build this list before so it doesn't get interfered with by
	// the transforms we have do do first.
	std::vector<IR::ParentedInsn<RetInsn>> returns;
	IR::BuildWorklist(returns, fn, HLIR::Return);

	// First always create a tail block - even if there is only
	// one BB in the function (therefore only one ret) it is still useful
	// for later passes to be able to assume the existence of a tail/epilogue
	// block.
	// Unnessesary BBs can be combined by later passes if nessesary (#FIXME)

	BasicBlock* tailBlock = BasicBlock::Create();
	fn->InsertBefore(fn->end(), tailBlock);

	// For non void return functions, allocate space on the stack to store the return value
	VirtualRegisterName* returnValueAddress = nullptr;
	const Type* returnType = fn->GetReturnType();

	if (!fn->IsVoidReturn()) {
		BasicBlock& headBlock  = *fn->begin();

		returnValueAddress = VirtualRegisterName::Create(BuiltinTypes::GetPointer());
		headBlock.InsertBefore(headBlock.begin(), Helix::CreateStackAlloc(returnValueAddress, returnType));

		// For non void functions we want to inject code into the tail block
		// to load the value from `returnValueAddres` and return it...

		VirtualRegisterName* returnValue = VirtualRegisterName::Create(returnType);

		tailBlock->InsertBefore(tailBlock->begin(), Helix::CreateLoad(returnValueAddress, returnValue));
		tailBlock->InsertAfter(tailBlock->begin(), Helix::CreateRet(returnValue));
	} else {
		// ... for void functions just return, no value to load.
		tailBlock->InsertBefore(tailBlock->begin(), Helix::CreateRet());
	}

	// Finally, now that the tail block has been created and is returning the value
	// in `returnValueAddress`, go though and replace each instance of ret
	// with a store to `returnValueAddress` and a branch to the tail block.

	for (IR::ParentedInsn<RetInsn>& insn : returns) {
		BasicBlock& bb = insn.parent;
		RetInsn& ret   = insn.insn;

		BasicBlock::iterator where = bb.Where(&ret);

		if (ret.HasReturnValue()) {
			where = bb.InsertAfter(where, Helix::CreateStore(ret.GetReturnValue(), returnValueAddress));
		}

		where = bb.InsertAfter(where, Helix::CreateUnconditionalBranch(tailBlock));

		bb.Delete(bb.Where(&ret));
	}
}

/*********************************************************************************************************************/

void CConv::Execute(Function* fn, const PassRunInformation&)
{
	// #FIXME: Maybe this can be simplified by assuming there is only one return?
	//         (as per the ReturnCombine pass)

	BasicBlock* tailBlock = fn->GetTailBlock();
	helix_assert(tailBlock, "function must have at least one block (and that should be its tail)");

	if (!tailBlock)
		return;

	RetInsn* ret = IR::FindFirstInstructionOfType<RetInsn>(*tailBlock, HLIR::Return);
	helix_assert(ret, "cannot find a return instruction in the tail block");

	if (!ret)
		return;

	PhysicalRegisterName* r0 = PhysicalRegisters::GetRegister(BuiltinTypes::GetInt32(), PhysicalRegisters::R0);
	const size_t r0Size = ARMv7::TypeSize(r0->GetType()); // always going to be 4, but meh

	if (ret->HasReturnValue()) {
		Value* returnValue = ret->GetReturnValue();

		// #FIXME: Need to implement returning values by "output" parameter.
		//         This is defined in AAPCS32
		helix_assert(ARMv7::TypeSize(returnValue->GetType()) <= r0Size, "return value can't fit in output register");

		IR::ReplaceAllUsesWith(returnValue, r0);

		ret->MakeVoid();
	
		// Set the type of this function to be void (now we've replaced all value returns)
		// *technically* this function still has a return type & is not void, but
		// at this point in the pipeline we need to lower past higher level details like that.

		const FunctionType* existingFunctionType = type_cast<FunctionType>(fn->GetType());
		helix_assert(existingFunctionType, "function type should be a FunctionType instance");

		const FunctionType* voidFunctionType = existingFunctionType->CopyWithDifferentReturnType(BuiltinTypes::GetVoidType());
		fn->SetType(voidFunctionType);	
	}
}

/*********************************************************************************************************************/

GlobalVariable* ConstantHoisting::CreateOrGetGlobal(Module* mod, ConstantInt* cint) {
	auto it = GlobalMap.find(cint);

	if (it == GlobalMap.end()) {
		static size_t index = 0;
		const std::string name = "ci" + std::to_string(index);
		index++;

		GlobalVariable* gvar = GlobalVariable::Create(name, cint->GetType(), cint);
		mod->RegisterGlobalVariable(gvar);

		GlobalMap.insert({cint, gvar});
	
		return gvar;
	}

	return it->second;
}

/*********************************************************************************************************************/

void ConstantHoisting::Execute(BasicBlock* bb, const PassRunInformation&)
{
	helix_assert(bb, "ConstantHoisting: NULl basic block");

	struct ConstantRef { Instruction& insn; };

	std::vector<ConstantRef> constantReferences;

	for (Instruction& insn : *bb) {
		for (size_t operandIndex = 0; operandIndex < insn.GetCountOperands(); ++operandIndex) {
			Value* pOperand = insn.GetOperand(operandIndex);

			if (pOperand->IsA<ConstantInt>()) {
				constantReferences.push_back({insn});
				break;
			}
		}
	}

	for (const ConstantRef& constantRef : constantReferences) {
		Instruction& insn = constantRef.insn;

		for (size_t operandIndex = 0; operandIndex < insn.GetCountOperands(); ++operandIndex) {
			Value* pOperand = insn.GetOperand(operandIndex);

			if (ConstantInt* cint = value_cast<ConstantInt>(pOperand)) {
				VirtualRegisterName* v = VirtualRegisterName::Create(pOperand->GetType());
				GlobalVariable* g = this->CreateOrGetGlobal(bb->GetParent()->GetParent(), cint);

				bb->InsertBefore(bb->Where(&insn), Helix::CreateLoad(g, v));

				insn.SetOperand(operandIndex, v);
			}
		}
	}
}

/*********************************************************************************************************************/

void LowerStructStackAllocation::Execute(Function* fn, const PassRunInformation&)
{
	BasicBlock* head = fn->GetHeadBlock();

	for (Instruction& insn : head->insns()) {
		if (insn.GetOpcode() == HLIR::StackAlloc) {
			StackAllocInsn& stack_alloc = (StackAllocInsn&) insn;
	
			if (stack_alloc.GetAllocatedType()->IsStruct()) {
				const size_t structSize = ARMv7::TypeSize(stack_alloc.GetAllocatedType());
				const ArrayType* arrayType = ArrayType::Create(structSize, BuiltinTypes::GetInt8());
				stack_alloc.SetAllocatedType(arrayType);
			}
		}
	}
}

/*********************************************************************************************************************/

void LegaliseStructs::CopyStruct(Value* src, Value* dst, const StructType* structType, BasicBlock::iterator where)
{
	// #FIXME: Maybe inserting a `memcpy` here can be better sometimes than a member wise copy?
	//         (it's what LLVM does)

	helix_assert(src->GetType()->IsPointer(), "source needs to be a pointer");
	helix_assert(dst->GetType()->IsPointer(), "destination needs to be a pointer");

	for (size_t fieldIndex = 0; fieldIndex < structType->GetCountFields(); ++fieldIndex) {
		VirtualRegisterName* sourceFieldAddress = VirtualRegisterName::Create(BuiltinTypes::GetPointer());
		VirtualRegisterName* destFieldAddress = VirtualRegisterName::Create(BuiltinTypes::GetPointer());

		VirtualRegisterName* value = VirtualRegisterName::Create(structType->GetField(fieldIndex));

		BasicBlock* bb = where->GetParent();

		where = bb->InsertAfter(where, Helix::CreateLoadFieldAddress(structType, src, (unsigned) fieldIndex, sourceFieldAddress));
		where = bb->InsertAfter(where, Helix::CreateLoadFieldAddress(structType, dst, (unsigned) fieldIndex, destFieldAddress));
		where = bb->InsertAfter(where, Helix::CreateLoad(sourceFieldAddress, value));
		where = bb->InsertAfter(where, Helix::CreateStore(value, destFieldAddress));
	}
}

/*********************************************************************************************************************/

void LegaliseStructs::Execute(Function* fn, const PassRunInformation&)
{
	struct LoadStore
	{
		LoadInsn*   load;
		std::vector<StoreInsn*> stores;
	};

	std::vector<LoadStore> load_stores;

	for (BasicBlock& bb : fn->blocks()) {
		for (Instruction& insn : bb.insns()) {
			if (insn.GetOpcode() == HLIR::Load) {
				LoadInsn& load = (LoadInsn&) insn;
				Value* dst = load.GetDst();

				if (dst->GetType()->IsStruct()) {
					LoadStore ls;
					ls.load = &load;

					bool usedInRet = false;

					for (const Use& use : dst->uses()) {
						Instruction* use_insn = use.GetInstruction();

						if (use_insn->GetOpcode() == HLIR::Store) {
							StoreInsn* store = (StoreInsn*) use_insn;
							ls.stores.push_back(store);
						}

						if (use_insn->GetOpcode() == HLIR::Return) {
							usedInRet = true;
						}
					}

					if (!usedInRet) {
						helix_assert(ls.stores.size() > 0, "expected at least one store of loaded struct");
						load_stores.push_back(ls);
					}
				}
			}
		}
	}

	for (LoadStore& ls : load_stores) {
		Value* source_ptr = ls.load->GetSrc();

		const StructType* struct_type = type_cast<StructType>(ls.load->GetDst()->GetType());
		helix_assert(struct_type, "type is not a struct!");

		for (StoreInsn* store : ls.stores) {
			helix_assert(struct_type == store->GetSrc()->GetType(), "source struct type != dest struct type");
			Value* dest_ptr = store->GetDst();

			BasicBlock* bb = store->GetParent();
			CopyStruct(source_ptr, dest_ptr, struct_type, bb->Where(store));

			store->DeleteFromParent();
		}

		ls.load->DeleteFromParent();
	}
}

/*********************************************************************************************************************/
