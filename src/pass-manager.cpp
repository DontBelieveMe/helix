#define DECLARE_PASS_IMPL

#include "pass-manager.h"
#include "module.h"
#include "print.h"
#include "options.h"

#include "lower.h"
#include "regalloc.h"
#include "match.h"
#include "validate.h"

using namespace Helix;

HELIX_DEFINE_LOG_CHANNEL(pass_manager);

PassManager::PassManager()
{
	AddPass<GenericLegalizer>();
	AddPass<ReturnCombine>();
	AddPass<GenericLowering>();
	AddPass<CConv>();
	AddPass<RegisterAllocator>();
	AddPass<FinalMatcher>();
}

void PassManager::Execute(Module* mod)
{
	ValidationPass validationPass;

	// Manually run a validation pass to check the IR emitted by the frontend.
	// Other validation passes will happen, but the pass manager can schedule them
	// automatically between passes.
	validationPass.Execute(mod);

	// Do this here since we want to run a validation pass first, but
	// don't actually want to run any other pass, just dump the frontend IR
	// and exit out.
	if (Options::GetEmitIR1()) {
		Helix::DebugDump(*mod);
	}

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

		// Automatically run a validation check after every pass. This is useful to ensure correctness
		// during development, but will definately slow things down in the long run (esp with bigger
		// source files). Maybe hide this behind a flag (--always-validate or something?)
		// #FIXME
		validationPass.Execute(mod);

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
