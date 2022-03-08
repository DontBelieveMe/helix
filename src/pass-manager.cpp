#define DECLARE_PASS_IMPL

/* Generic Internal Project Includes */
#include "pass-manager.h"
#include "module.h"
#include "print.h"
#include "options.h"

/* Pass Internal Project Includes */
#include "lower.h"
#include "regalloc.h"
#include "match.h"
#include "validate.h"
#include "emit.h"
#include "genlower.h"
#include "regalloc2.h"
#include "arm-split-constants.h"
#include "peephole-generic.h"
#include "mem2reg.h"
#include "scp.h"
#include "dce.h"

using namespace Helix;

HELIX_DEFINE_LOG_CHANNEL(pass_manager);

/*********************************************************************************************************************/

PassManager::PassManager()
{
	/* Generic Passes */
	AddPass<GenericLegalizer>();
	AddPass<LegaliseStructs>();
	AddPass<ReturnCombine>();
	AddPass<GenericLowering>();
	AddPass<Mem2Reg>();
	AddPass<PeepholeGeneric>();
	AddPass<SCP>();
	AddPass<DCE>();

	/* ARM Specific Passes */
	AddPass<CConv>();
	AddPass<LowerStructStackAllocation>();
	AddPass<ArmSplitConstants>();
	AddPass<MachineExpander>();
	AddPass<RegisterAllocator2>();
	AddPass<AssemblyEmitter>();
}

/*********************************************************************************************************************/

void PassManager::ValidateModule(ValidationPass& validationPass, Module* module)
{
	HELIX_PROFILE_ZONE;
	
	validationPass.Execute(module, {});
}

/*********************************************************************************************************************/

void PassManager::RunPass(const PassData& passData, Module* module)
{
	HELIX_PROFILE_ZONE;
	HELIX_PROFILE_ZONE_TEXT(passData.name, strlen(passData.name));

	std::unique_ptr<Pass> pass = passData.create_action();

	helix_trace(logs::pass_manager, "Pass: {}", passData.name);

	if (Options::GetEmitIRPrePass() == passData.name) {
		Helix::DebugDump(*module);
	}

	PassRunInformation info;
	info.TestTrace = (Options::GetTestTracePass() == passData.name);

	pass->Execute(module, info);

	if (Options::GetEmitIRPostPass() == passData.name) {
		Helix::DebugDump(*module);
	}

	if (Options::GetDumpCFGPost() == passData.name) {
		module->DumpControlFlowGraphToFile(fmt::format("cfg-{}.dot", passData.name));
	}
}

/*********************************************************************************************************************/

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
		// during development, but will definitely slow things down in the long run (esp with bigger
		// source files). Maybe hide this behind a flag (--always-validate or something?)
		// #FIXME
		this->ValidateModule(validationPass, mod);

		if (Options::GetStopAfterPass() == passData.name) {
			helix_warn(logs::pass_manager, "Aborting, due to --stop-after-pass={}", Options::GetStopAfterPass());
			break;
		}
	}
}

/*********************************************************************************************************************/

void BasicBlockPass::Execute(Module* mod, const PassRunInformation& info)
{
	for (auto it = mod->functions_begin(); it != mod->functions_end(); ++it) {
		Function* fn = *it;

		for (auto bbit = fn->begin(); bbit != fn->end(); ++bbit) {
			BasicBlock& bb = *bbit;
			this->Execute(&bb, info);
		}
	}
}

/*********************************************************************************************************************/

void FunctionPass::Execute(Module* mod, const PassRunInformation& info)
{
	for (auto it = mod->functions_begin(); it != mod->functions_end(); ++it) {
		Function* fn = *it;
		this->Execute(fn, info);
	}
}

/*********************************************************************************************************************/

