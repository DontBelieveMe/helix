/**
 * @file emit.cpp
 * @author Barney Wilks
 * 
 * This handles the final process of taking the final IR form and converting it
 * to assembly.
 * Because it's simpler, this project does not (currently) implement an assembler or
 * linker and those functionalities are expected to be provided by an external toolchain
 * (the GCC driver assemble and link easily for us, just run `gcc <assemblyfile> ...` as
 * you would with any other source file).
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
 *         and is safer than using printf style formatting).
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

#include "emit.h"
#include "options.h"
#include "module.h"
#include "print.h"
#include "helix.h"
#include "mir.h"

using namespace Helix;

/*********************************************************************************************************************/

static const char* GetAssemblyDirectiveForType(const Type* type)
{
	if (type->IsPointer()) {
		return "4byte";
	}

	if (const IntegerType* int_type = type_cast<IntegerType>(type)) {
		switch (int_type->GetBitWidth()) {
		case 8:  return "byte";
		case 16: return "2byte";
		case 32: return "4byte";
		case 64: return "8byte";
		default:
			helix_unreachable("no assembly directive for integral type of this size");
			break;
		}
	}

	if (const ArrayType* array_type = type_cast<ArrayType>(type)) {
		return GetAssemblyDirectiveForType(array_type->GetBaseType());
	}

	helix_unreachable("no assembly directive for unknown type");
	return nullptr;
}

/*********************************************************************************************************************/

static const char* GetAssemblyDirectiveForValue(Value* value)
{
	if (ConstantByteArray* cba = value_cast<ConstantByteArray>(value)) {
		if (cba->IsString()) {
			return "ascii"; // we include the null terminator ourselves so don't use .asciz
		}
	}

	return GetAssemblyDirectiveForType(value->GetType());
}

/*********************************************************************************************************************/

static void EmitDataDirective(FILE* file, Value* init_value)
{
	const char* label = GetAssemblyDirectiveForValue(init_value);

	fprintf(file, "\t.%s ", label);

	if (ConstantInt* constIntExpr = value_cast<ConstantInt>(init_value)) {
		fprintf(file, "%llu\n", constIntExpr->GetIntegralValue());
	}
	else if (ConstantByteArray* constByteArray = value_cast<ConstantByteArray>(init_value)) {
		if (constByteArray->IsString()) {
			fprintf(file, "\"");
			for (uint8_t byte : constByteArray->bytes()) {
				if (isprint(byte)) {
					fprintf(file, "%c", byte);
				}
				else {
					fprintf(file, "\\%x", byte);
				}
			}
			fprintf(file, "\"\n");
		}
		else {
			helix_unimplemented("can't print ConstantByteArray global initializer");
		}
	}
	else if (GlobalVariable* global = value_cast<GlobalVariable>(init_value)) {
		fprintf(file, "%s\n", global->GetName());
	}
	else {
		helix_unimplemented("can't print constant initializer expression");
	}
}

/*********************************************************************************************************************/

void AssemblyEmitter::Execute(Module* mod, const PassRunInformation&)
{
	const std::string& assemblyFileName = Helix::GetAssemblyOutputFilePath(mod);

	FILE* file = [&assemblyFileName]() -> FILE* {
		// "-o -" is a useful way to output to stdout (nice for debugging)
		if (assemblyFileName == "-") {
			return stdout;
		} else {
			return fopen(assemblyFileName.c_str(), "w");
		}
	}();

	helix_info(logs::emit, "Ouputting assembly to '{}'", assemblyFileName);

	helix_assert(file, "failed to open output assembly file");

	SlotTracker slots;

	// First go through and print all the global variables at the top of the file...

	fprintf(file, ".section .data\n");

	for (GlobalVariable* global : mod->globals()) {
		Value* init = global->GetInit();
		if (init) {
			fprintf(file, "%s:\n", global->GetName());
			
			if (ConstantStruct* constant_struct = value_cast<ConstantStruct>(init)) {
				for (size_t i = 0; i < constant_struct->GetCountValues(); ++i) {
					EmitDataDirective(file, constant_struct->GetValue(i));
				}
			} else {
				EmitDataDirective(file, init);
			}
		}
		else {
			const size_t sizeInBytes = ARMv7::TypeSize(global->GetBaseType());
			fprintf(file, "%s:\n\t.space %llu\n", global->GetName(), sizeInBytes);
		}
	}

	fprintf(file, ".text\n");

	// Then print the actual function (code) itself...

	for (Function* fn : mod->functions()) {
		const std::string& functionName = fn->GetName();

		if (!fn->HasBody()) {
			fprintf(file, ".globl %s\n", functionName.c_str());
			continue;
		}

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

		fprintf(file, "\tpush {r4, r5, r6, r7, r8, r10, r11, lr}\n"); 
		fprintf(file, "\tmov r11, sp\n");

		for (BasicBlock& bb : fn->blocks()) {

			// #FIXME: Not every basic block will need a label, maybe a simple check
			//         of uses would do? Not emitting unnecessary labels makes the assembly
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

	// Make sure to close the file (as long as we haven't been printing to stdout,
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
