#include "pass-manager.h"
#include "module.h"
#include "print.h"
#include "options.h"

#include "lower.h"
#include "regalloc.h"
#include "match.h"

using namespace Helix;

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
	for (const PassData& passData : m_Passes) {
		std::unique_ptr<Pass> pass = passData.create_action();
		pass->Execute(mod);

		if (Options::GetEmitIRPostPass() == passData.name) {
			Helix::DebugDump(*mod);
		}
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
