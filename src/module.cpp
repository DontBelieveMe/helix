/**
 * @file module.cpp
 * @author Barney Wilks
 *
 * Implements module.h
 */

#if defined(_MSC_VER)
	#define _CRT_SECURE_NO_WARNINGS
#endif

/* Internal Project Includes */
#include "module.h"
#include "system.h"
#include "print.h"

/* C Standard Library Includes */
#include <stdio.h>

using namespace Helix;

/******************************************************************************/

Module::Module(const std::string& inputSourceFile)
    : m_InputSourceFile(inputSourceFile)
{ }

/******************************************************************************/

void
Module::RegisterFunction(Function* fn)
{
	fn->SetParent(this);
	m_Functions.push_back(fn);
}

/******************************************************************************/

void
Module::RegisterStruct(const StructType* ty)
{
	m_Structs.push_back(ty);
}

/******************************************************************************/

void
Module::RegisterGlobalVariable(GlobalVariable* gvar)
{
	m_GlobalVariables.push_back(gvar);
}

/******************************************************************************/

Function*
Module::FindFunctionByName(const std::string& name) const
{
	for (Function* fn : m_Functions) {
		if (fn->GetName() == name)
			return fn;
	}

	return nullptr;
}

/******************************************************************************/

static void
PrintBasicBlockNode(SlotTracker& fnSlots, FILE* file, BasicBlock& bb)
{
	fprintf(file, "\"");

	TextOutputStream stream(file);
	Print(fnSlots, stream, bb);

	fprintf(file, "\"");
}

/******************************************************************************/

void
Module::DumpControlFlowGraphToFile(const std::string& filepath)
{
	FILE* file = fopen(filepath.c_str(), "w");

	helix_debug(logs::general, "Dumping module CFG to graph '{}'", filepath);

	if (!file) {
		helix_warn(
		    logs::general,
		    "Couldn't open file '{}' for writing, skipping CFG dump", filepath
		);

		return;
	}

	fprintf(file, "digraph module {\n"
	              "\tlabeljust=l;\n"
	              "\tnojustify=true;\n"
	              "\tnodesep=1;\n");

	for (Function* fn : functions()) {
		SlotTracker slots;

		const std::string& name = fn->GetName();
		const char* returnTypeString = GetTypeName(fn->GetReturnType());

		fprintf(file, "\tsubgraph cluster%s {\n", name.c_str());

		{
			fprintf(file, "\t\tlabel = \"%s(", name.c_str());

			for (auto it = fn->params_begin(); it != fn->params_end(); ++it) {
				fprintf(file, "%s", GetTypeName((*it)->GetType()));

				if (it < fn->params_end() - 1)
					fprintf(file, ", ");
			}

			fprintf(file, "): %s\";\n", returnTypeString);
		}

		fprintf(file, "\t\tcolor = black;\n");

		for (BasicBlock& bb : fn->blocks()) {
			const Instruction* term = bb.GetTerminator();
			helix_assert(term, "Block must have terminator for CFG dump");

			if (!term)
				continue;

			PrintBasicBlockNode(slots, file, bb);

			switch (term->GetOpcode()) {
			case HLIR::ConditionalBranch: {
				const ConditionalBranchInsn* cbr = (const ConditionalBranchInsn*) term;

				fprintf(file, " -> ");
				PrintBasicBlockNode(slots, file, *cbr->GetTrueBB());
				fprintf(file, "\n");

				PrintBasicBlockNode(slots, file, bb);
				fprintf(file, " -> ");
				PrintBasicBlockNode(slots, file, *cbr->GetFalseBB());

				break;
			}

			case HLIR::UnconditionalBranch: {
				const UnconditionalBranchInsn* br
					= (const UnconditionalBranchInsn*) term;

				fprintf(file, " -> ");
				PrintBasicBlockNode(slots, file, *br->GetBB());

				break;
			}

			default:
				break;
			}

			fprintf(file, ";\n");
		}

		fprintf(file, "\t}\n");
	}

	fprintf(file, "}\n");
	fclose(file);
}

/******************************************************************************/

Module*
Helix::CreateModule(const std::string& inputSourceFile)
{
	return new Module(inputSourceFile);
}

/******************************************************************************/
