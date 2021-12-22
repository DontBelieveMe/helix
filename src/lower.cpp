#include "lower.h"
#include "system.h"
#include "function.h"
#include "instructions.h"
#include "target-info-armv7.h"
#include "module.h"
#include "print.h"

#include "arm-md.h" /* generated */

#include <vector>
#include <numeric>

// #pragma optimize("", off)

using namespace Helix;

namespace Helix::ARMv7
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
				size_t(0),
				[](size_t v, const Type* field) -> size_t {
					return v + ARMv7::TypeSize(field);
				}
			);
		}

		default:
			helix_unimplemented("TypeSize not implemented for type category");
			break;
		}

		return 0;
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

	template <typename T>
	struct ParentedInsn
	{
		T& insn;
		BasicBlock& parent;
	};

	template <typename T>
	static void BuildWorklist(std::vector<ParentedInsn<T>>& insns, Function* fn, Opcode opcode)
	{
		for (BasicBlock& bb : fn->blocks()) {
			for (Instruction& insn : bb.insns()) {
				if (insn.GetOpcode() == opcode) {
					insns.push_back({ static_cast<T&>(insn), bb });
				}
			}
		}
	}

	template <typename T>
	static T* FindFirstInstructionOfType(BasicBlock& bb, Opcode opcode) {
		for (Instruction& insn : bb) {
			if (insn.GetOpcode() == opcode) {
				return static_cast<T*>(&insn);
			}
		}

		return nullptr;
	}
}

void GenericLowering::LowerLea(BasicBlock& bb, LoadEffectiveAddressInsn& insn)
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

	helix_debug(logs::genlower, "Found {0} instructions that require lowering", worklist.size());

	for (const WorkPair& workload : worklist) {
		switch (workload.insn->GetOpcode()) {
		case kInsn_LoadElementAddress:
			this->LowerLea(*workload.bb, *static_cast<LoadEffectiveAddressInsn*>(workload.insn));
			break;

		case kInsn_LoadFieldAddress:
			this->LowerLfa(*workload.bb, *static_cast<LoadFieldAddressInsn*>(workload.insn));
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

void GenericLegalizer::Execute(Function* fn)
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
				case kInsn_Store: {
					StoreInsn& store = static_cast<StoreInsn&>(insn);

					if (value_isa<ConstantArray>(store.GetSrc()) || value_isa<ConstantStruct>(store.GetSrc())) {
						illegalStores.push_back({store, bb });
					}

					break;
				}
				case kInsn_StackAlloc: {
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

		dirty = !illegalStores.empty();
	} while (dirty);
}

void ReturnCombine::Execute(Function* fn)
{
	helix_assert(fn->GetCountBlocks() >= 1, "Function must have at least one basic block");

	// Build this list before so it doesn't get interfered with by
	// the transforms we have do do first.
	std::vector<IR::ParentedInsn<RetInsn>> returns;
	IR::BuildWorklist(returns, fn, kInsn_Return);

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

void CConv::Execute(Function* fn)
{
	// #FIXME: Maybe this can be simplified by assuming there is only one return?
	//         (as per the ReturnCombine pass)

	BasicBlock* tailBlock = fn->GetTailBlock();
	helix_assert(tailBlock, "function must have at least one block (and that should be its tail)");

	if (!tailBlock)
		return;

	RetInsn* ret = IR::FindFirstInstructionOfType<RetInsn>(*tailBlock, kInsn_Return);
	helix_assert(ret, "cannot find a return instruction in the tail block");

	if (!ret)
		return;

	PhysicalRegisterName* r0 = PhysicalRegisters::GetRegister(PhysicalRegisters::R0);
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

void ConstantHoisting::Execute(BasicBlock* bb)
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

unsigned Align(unsigned input, unsigned alignment)
{
	if (input % alignment == 0) {
		return input;
	}

	return input + (alignment - (input % alignment));
}

/*********************************************************************************************************************/

void LowerStackAllocations::ComputeStackFrame(StackFrame& frame, Function* fn)
{
	frame.size = 0;

	// At this point all stack allocations should be in the first block
	// so we don't need to scan any other blocks in the function.

	BasicBlock* head = fn->GetHeadBlock();

	for (Instruction& insn : head->insns()) {
		if (insn.GetOpcode() == kInsn_StackAlloc) {
			StackAllocInsn& stack_alloc = (StackAllocInsn&) insn;

			const Type*  allocated_type = stack_alloc.GetAllocatedType();
			const size_t allocated_size = ARMv7::TypeSize(allocated_type);

			frame.size += (unsigned) allocated_size;

			const StackVariable stack_variable {
				(unsigned) allocated_size,
				frame.size,
				&stack_alloc
			};

			frame.variables.push_back(stack_variable);
		}
	}

	frame.size = Align(frame.size, 8);
}

/*********************************************************************************************************************/

void LowerStackAllocations::Execute(Function* fn)
{
	StackFrame stack_frame;
	this->ComputeStackFrame(stack_frame, fn);

	BasicBlock* head = fn->GetHeadBlock();
	BasicBlock* tail = fn->GetTailBlock();

	helix_assert(head && tail, "requires head & tail blocks");

	PhysicalRegisterName* sp = PhysicalRegisters::GetRegister(PhysicalRegisters::SP);

	for (const StackVariable& stack_var : stack_frame.variables) {
		const unsigned offset = stack_frame.size - stack_var.offset;

		BasicBlock::iterator where = head->Where(stack_var.alloca_insn);

		ConstantInt*         offset_value = ConstantInt::Create(BuiltinTypes::GetInt32(), offset);
		VirtualRegisterName* temp         = VirtualRegisterName::Create(BuiltinTypes::GetInt32());
		Value*               output_ptr   = stack_var.alloca_insn->GetOutputPtr();

		where = head->InsertAfter(where, Helix::CreateBinOp(kInsn_IAdd, sp, offset_value, temp));
		where = head->InsertAfter(where, Helix::CreateIntToPtr(BuiltinTypes::GetInt32(), temp, output_ptr));

		head->Remove(head->Where(stack_var.alloca_insn));
	}

	ConstantInt* stack_size_constant = ConstantInt::Create(BuiltinTypes::GetInt32(), stack_frame.size);

	head->InsertBefore(head->begin(), Helix::CreateBinOp(kInsn_ISub, sp, stack_size_constant, sp));
	tail->InsertBefore(tail->begin(), Helix::CreateBinOp(kInsn_IAdd, sp, stack_size_constant, sp));
}

/*********************************************************************************************************************/
