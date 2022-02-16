#include "arm-split-constants.h"
#include "arm-md.h" /* generated */
#include "function.h"
#include "ir-helpers.h"
#include "mir.h"

using namespace Helix;

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

		helix_assert(type_cast<IntegerType>(integer->GetType())->GetBitWidth() == 32, "Only 32 bit integers are supported :(");

		const Helix::Integer fullValue = integer->GetIntegralValue();

		const Helix::Integer bottomHalf = fullValue & 0xffff;
		const Helix::Integer topHalf    = fullValue >> 16;

		VirtualRegisterName* result = VirtualRegisterName::Create(BuiltinTypes::GetInt32());

		MachineInstruction* movw = ARMv7::CreateMovwi(result, ConstantInt::Create(BuiltinTypes::GetInt32(), bottomHalf));
		MachineInstruction* movt = ARMv7::CreateMovti(result, ConstantInt::Create(BuiltinTypes::GetInt32(), topHalf));

		IR::InsertBefore(ref.Insn, movt);
		IR::InsertBefore(movt, movw);

		ref.UseRef.ReplaceWith(result);
	}
}
