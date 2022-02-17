#include "arm-split-constants.h"
#include "arm-md.h" /* generated */
#include "function.h"
#include "ir-helpers.h"
#include "mir.h"

using namespace Helix;

/*********************************************************************************************************************/

static VirtualRegisterName* GetIntegerIntoRegister_32(Instruction* user, ConstantInt* integer)
{
	helix_assert(type_cast<IntegerType>(integer->GetType())->GetBitWidth() == 32, "Expects 32 bit integers :(");

	const Helix::Integer fullValue = integer->GetIntegralValue();

	const Helix::Integer bottomHalf = fullValue & 0xffff;
	const Helix::Integer topHalf = fullValue >> 16;

	VirtualRegisterName* result = VirtualRegisterName::Create(BuiltinTypes::GetInt32());

	MachineInstruction* movw = ARMv7::CreateMovwi(result, ConstantInt::Create(BuiltinTypes::GetInt32(), bottomHalf));
	MachineInstruction* movt = ARMv7::CreateMovti(result, ConstantInt::Create(BuiltinTypes::GetInt32(), topHalf));

	IR::InsertBefore(user, movt);
	IR::InsertBefore(movt, movw);

	return result;
}

/*********************************************************************************************************************/

static VirtualRegisterName* GetIntegerIntoRegister_16(Instruction* user, ConstantInt* integer)
{
	helix_assert(type_cast<IntegerType>(integer->GetType())->GetBitWidth() == 16, "Expects 16 bit integers :(");

	const Helix::Integer fullValue = integer->GetIntegralValue();

	const Helix::Integer bottomHalf = fullValue & 0xffff;
	const Helix::Integer topHalf = 0;

	VirtualRegisterName* result = VirtualRegisterName::Create(BuiltinTypes::GetInt32());

	MachineInstruction* movw = ARMv7::CreateMovwi(result, ConstantInt::Create(BuiltinTypes::GetInt32(), bottomHalf));
	MachineInstruction* movt = ARMv7::CreateMovti(result, ConstantInt::Create(BuiltinTypes::GetInt32(), topHalf));

	IR::InsertBefore(user, movt);
	IR::InsertBefore(movt, movw);

	return result;
}

/*********************************************************************************************************************/

static VirtualRegisterName* GetIntegerIntoRegister_8(Instruction* user, ConstantInt* integer)
{
	helix_assert(type_cast<IntegerType>(integer->GetType())->GetBitWidth() == 8, "Expects 8 bit integers :(");

	const Helix::Integer fullValue = integer->GetIntegralValue();

	const Helix::Integer bottomHalf = fullValue & 0x00ff;
	const Helix::Integer topHalf    = 0;

	VirtualRegisterName* result = VirtualRegisterName::Create(BuiltinTypes::GetInt32());

	MachineInstruction* movw = ARMv7::CreateMovwi(result, ConstantInt::Create(BuiltinTypes::GetInt32(), bottomHalf));
	MachineInstruction* movt = ARMv7::CreateMovti(result, ConstantInt::Create(BuiltinTypes::GetInt32(), topHalf));

	IR::InsertBefore(user, movt);
	IR::InsertBefore(movt, movw);

	return result;
}

/*********************************************************************************************************************/

void ArmSplitConstants::Execute(Function* fn, const PassRunInformation& info)
{
	struct IntegerReference
	{
		Instruction* Insn;
		ConstantInt* Value;
		Use          UseRef;
	};

	std::vector<IntegerReference> constantIntegers;

	for (BasicBlock& bb : fn->blocks()) {
		for (Instruction& insn : bb) {
			for (size_t opIndex = 0; opIndex < insn.GetCountOperands(); ++opIndex) {
				if (ConstantInt* integerValue = value_cast<ConstantInt>(insn.GetOperand(opIndex))) {
					constantIntegers.push_back({ &insn, integerValue, Use(&insn, opIndex)});
				}
			}
		}
	}

	for (IntegerReference& ref : constantIntegers) {
		ConstantInt* integer = ref.Value;

		const IntegerType* type = type_cast<IntegerType>(integer->GetType());
		VirtualRegisterName* result = nullptr;
		switch (type->GetBitWidth()) {
		case 32: result = GetIntegerIntoRegister_32(ref.Insn, integer); break;
		case 16:  result = GetIntegerIntoRegister_16(ref.Insn, integer); break;
		case 8: result = GetIntegerIntoRegister_8(ref.Insn, integer); break;
		default:
			helix_unreachable("Unsupported bit width for this operation :(");
			break;
		}

		ref.UseRef.ReplaceWith(result);
	}
}

/*********************************************************************************************************************/
