#define DECLARE_PASS_IMPL

#include "pass-manager.h"
#include "module.h"
#include "print.h"
#include "options.h"

#include "lower.h"
#include "regalloc.h"
#include "match.h"
#include "validate.h"
#include "emit.h"

using namespace Helix;

HELIX_DEFINE_LOG_CHANNEL(pass_manager);

PassManager::PassManager()
{
	/* Generic Passes */
	AddPass<GenericLegalizer>();
	AddPass<LegaliseStructs>();
	AddPass<ReturnCombine>();
	AddPass<GenericLowering>();

	/* ARM Specific Passes */
	AddPass<ConstantHoisting>();
	AddPass<CConv>();
	AddPass<LowerStructStackAllocation>();
	AddPass<RegisterAllocator>();
	AddPass<MachineExpander>();
	AddPass<AssemblyEmitter>();
}

void PassManager::ValidateModule(ValidationPass& validationPass, Module* module)
{
	HELIX_PROFILE_ZONE;
	
	validationPass.Execute(module);
}

void PassManager::RunPass(const PassData& passData, Module* module)
{
	HELIX_PROFILE_ZONE;
	HELIX_PROFILE_ZONE_TEXT(passData.name, strlen(passData.name));

	std::unique_ptr<Pass> pass = passData.create_action();

	helix_trace(logs::pass_manager, "Pass: {}", passData.name);

	if (Options::GetEmitIRPrePass() == passData.name) {
		Helix::DebugDump(*module);
	}

	pass->Execute(module);

	if (Options::GetEmitIRPostPass() == passData.name) {
		Helix::DebugDump(*module);
	}

	if (Options::GetDumpCFGPost() == passData.name) {
		module->DumpControlFlowGraphToFile(fmt::format("cfg-{}.dot", passData.name));
	}
}

void PassManager::Execute(Module* mod)
{
	HELIX_PROFILE_ZONE;

	ValidationPass validationPass;

	if (Options::GetEmitIR1()) {
		Helix::DebugDump(*mod);
	}

	// Manually run a validation pass to check the IR emitted by the frontend.
	// Other validation passes will happen, but the pass manager can schedule them
	// automatically between passes.
	this->ValidateModule(validationPass, mod);

	for (const PassData& passData : m_Passes) {
		this->RunPass(passData, mod);

		helix_trace(logs::pass_manager, "Validating pass '{}'", passData.name);

		// Automatically run a validation check after every pass. This is useful to ensure correctness
		// during development, but will definately slow things down in the long run (esp with bigger
		// source files). Maybe hide this behind a flag (--always-validate or something?)
		// #FIXME
		this->ValidateModule(validationPass, mod);
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
