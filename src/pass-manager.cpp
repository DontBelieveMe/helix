#define DECLARE_PASS_IMPL

#include "pass-manager.h"
#include "module.h"
#include "print.h"
#include "options.h"

#include "lower.h"
#include "regalloc.h"
#include "match.h"

using namespace Helix;

HELIX_DEFINE_LOG_CHANNEL(pass_manager);

PassManager::PassManager()
{
	AddPass<GenericLegalizer>();
	AddPass<GenericLowering>();
	AddPass<CConv>();
	AddPass<RegisterAllocator>();
	AddPass<FinalMatcher>();
}

void PassManager::Execute(Module* mod)
{
	size_t runIndex = 1;
	for (const PassData& passData : m_Passes) {
		std::unique_ptr<Pass> pass = passData.create_action();

		helix_debug(logs::pass_manager, "({}) Running pass '{}'...", runIndex, passData.name);
		pass->Execute(mod);

		if (Options::GetEmitIRPostPass() == passData.name) {
			Helix::DebugDump(*mod);
		}

		if (Options::GetDumpCFGPost() == passData.name) {
			mod->DumpControlFlowGraphToFile(fmt::format("cfg-{}.dot", passData.name));
		}

		runIndex++;
	}
}
	
void BasicBlockPass::Execute(Module* mod)
{
	for (auto it = mod->functions_begin(); it != mod->functions_end(); ++it) {
		Function* fn = *it;

		for (auto bbit = fn->begin(); bbit != fn->end(); ++bbit) {
			BasicBlock& bb = *bbit;
			this->Execute(&bb);
		}
	}
}

void FunctionPass::Execute(Module* mod)
{
	for (auto it = mod->functions_begin(); it != mod->functions_end(); ++it) {
		Function* fn = *it;
		this->Execute(fn);
	}
}
