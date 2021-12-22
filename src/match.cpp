#if defined(_MSC_VER)
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "match.h"
#include "options.h"
#include "module.h"
#include "print.h"

#include "arm-md.h"

using namespace Helix;

static void Emit(FILE* output, Instruction& insn, SlotTracker& slots)
{
	Helix::ARMv7::Emit(output, insn, slots);
}

void FinalMatcher::Execute(Module* mod)
{
	// temp
	if (!Options::GetEmitOnlyAssembly())
		return;

	const std::string& assemblyFileName = Options::GetOutputAssemblyFile();

	FILE* file;

	if (assemblyFileName.empty() || assemblyFileName == "-")
		file = stdout;
	else
		file = fopen(assemblyFileName.c_str(), "w");

	SlotTracker slots;

	for (GlobalVariable* global : mod->globals()) {
		ConstantInt* init = value_cast<ConstantInt>(global->GetInit());
		helix_assert(init->GetType() == BuiltinTypes::GetInt32(), "only 32 bit integer constants supported");
		helix_assert(init, "only user defined initialisers are supported");

		fprintf(file, "%s: .long %llu\n", global->GetName(), init->GetIntegralValue());
	}

	for (Function* fn : mod->functions()) {
		const std::string& functionName = fn->GetName();
		fprintf(file, ".globl %s\n%s:\n", functionName.c_str(), functionName.c_str());

		for (BasicBlock& bb : fn->blocks()) {
			const size_t bbSlot = slots.GetBasicBlockSlot(&bb);
			fprintf(file, ".bb%zu:\n", bbSlot);

			for (Instruction& insn : bb.insns()) {
				Emit(file, insn, slots);
			}
		}
	}

	if (file != stdout)
		fclose(file);
}