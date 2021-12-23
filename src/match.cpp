/**
 * @file match.cpp
 * @author Barney Wilks
 * 
 * This handles the final process of taking the final IR form and converting it
 * to assembly.
 * Because it's simpler, this project does not (currently) implement an assembler or
 * linker and those functionalies are expected to be provided by an external toolchain
 * (the GCC driver assemble and link easily for us, just run `gcc <assemblyfile> ...` as
 * you would with any other soruce file).
 * 
 * As a result the compiler needs to produce textual assembly. This assembly can either
 * get written to a file (as would be expected during normal compilation), or to stdout
 * (if the output file is given as '-' or currently if no output file is provided - this
 * stdout by default behaviour is currently just for development purposes, and eventually
 * a temp file should be created if none is specified).
 * 
 * #FIXME: Currently uses the LibC FILE* I/O mechanisms, but that's not great.
 *         Seem that fmtlib has it's own file IO systems, which would be great
 *         as fmtlib is already heavily in use looks to have quite good performance
 *         and is safter than using printf style formatting).
 *         Initial attempts at this have been unsuccessful, but would be good to have
 *         another look (maybe using our own fmtlib dependency instead of relying on the
 *         one packed in spdlog. Windows support also seemed to be a bit in the air, so
 *         double check on that).
 */

// Do this before any other include, to make sure it's defined when fopen gets
// pulled in.
#if defined(_MSC_VER)
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "match.h"
#include "options.h"
#include "module.h"
#include "print.h"

#include "arm-md.h" /* generated */

using namespace Helix;

/*********************************************************************************************************************/

void FinalMatcher::Execute(Module* mod)
{
	if (!Options::GetEmitOnlyAssembly()) {
		return;
	}

	const std::string& assemblyFileName = Options::GetOutputAssemblyFile();

	FILE* file = [&assemblyFileName]() -> FILE* {
		// If -S is given, and -o is either not specified or equals "-" (a GCC convention)
		// then print to stdout.
		// Otherwise we want to create a new file and print to that.

		if (assemblyFileName.empty() || assemblyFileName == "-") {
			return stdout;
		} else {
			return fopen(assemblyFileName.c_str(), "w");
		}
	}();

	SlotTracker slots;

	// First go through and print all the global variables at the top of the file...

	for (GlobalVariable* global : mod->globals()) {
		ConstantInt* init = value_cast<ConstantInt>(global->GetInit());
		helix_assert(init && init->GetType() == BuiltinTypes::GetInt32(), "only 32 bit integer constants with initializers");

		fprintf(file, "%s: .long %llu\n", global->GetName(), init->GetIntegralValue());
	}

	// Then print the actual function (code) itself...

	for (Function* fn : mod->functions()) {
		const std::string& functionName = fn->GetName();

		// #FIXME: Not every function has external linkage (and hence wants this)
		//         Add some attribute to Function to signify linkage
		fprintf(file, ".globl %s\n%s:\n", functionName.c_str(), functionName.c_str());

		// -----------------------
		//  | Function prologue |
		// -----------------------
		//
		// Print this at the top of the function (before the first basic block)
		// Don't want to print this merged into the first basic block in case
		// anything branches to the top of the head BB (we don't want this code to
		// be executed more than once.
		//
		// #FIXME: There is probably something a bit smarter that could be done here
		//         around basic blocks existing that don't need to/we can detect if the
		//         head BB is used as a branch target etc...
		//         Not much point in printing BB labels if they are never used...

		fprintf(file, "\tpush {r11, lr}\n");
		fprintf(file, "\tmov r11, sp\n");

		for (BasicBlock& bb : fn->blocks()) {

			// #FIXME: Not every basic block will need a label, maybe a simple check
			//         of uses would do? Not emitting unnessesary labels makes the assembly
			//         a bit cleaner to read and less text for the assembler to process is not
			//         going to be a bad thing.
			const size_t basicBlockSlotIndex = slots.GetBasicBlockSlot(&bb);
			fprintf(file, ".bb%zu:\n", basicBlockSlotIndex);

			for (Instruction& insn : bb.insns()) {
				// Shell out to the generated code (generated from arm.md) to pattern match
				// this instruction against an assembly pattern, and then print it to the given file.
				// This will assert if a instruction can't be matched.
				//
				// #FIXME: Is this the best behaviour?

				ARMv7::Emit(file, insn, slots);
			}
		}

		// The function epilogue is emitted by the "ret" instruction pattern in arm.md
		// and is not hardcoded here (like the prologue is)
	}

	// Make sure to close the file (as long as we havn't been printing to stdout,
	// closing stdout would be bad...?? probably UB)
	//
	// #FIXME: Maybe find a better way of doing this (perhaps a `closeFile` flag or something).
	//         This seems a bit flakey, since there might be other FILE* pointers that we don't
	//         want to close.

	if (file != stdout || file == stderr) {
		fclose(file);
	}
}

/*********************************************************************************************************************/
