#include "match.h"
#include "options.h"
#include "module.h"
#include "print.h"

#include "arm-md.h"

using namespace Helix;

static void Emit(FILE* output, Instruction& insn)
{
	Helix::ARMv7::Emit(output, insn);
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

	for (Function* fn : mod->functions()) {
		const std::string& functionName = fn->GetName();
		fprintf(file, ".globl %s\n%s:\n", functionName.c_str(), functionName.c_str());

		for (BasicBlock& bb : fn->blocks()) {
			const size_t bbSlot = slots.GetBasicBlockSlot(&bb);
			fprintf(file, ".%zu:\n", bbSlot);

			for (Instruction& insn : bb.insns()) {
				Emit(file, insn);
			}
		}
	}

	if (file != stdout)
		fclose(file);
}