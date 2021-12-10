#include "pass-manager.h"

#include "module.h"

#include "lower.h"

using namespace Helix;

PassManager::PassManager()
{
    AddPass<GenericLowering>();
}

void PassManager::Execute(Module* mod)
{
    for (const CreatePassFunctor& createPass : m_Passes) {
        std::unique_ptr<Pass> pass = createPass();
        pass->Execute(mod);
    }
}

void FunctionPass::Execute(Module* mod)
{
    for (auto it = mod->functions_begin(); it != mod->functions_end(); ++it) {
        Function* fn = *it;
        this->Execute(fn);
    }
}
