#pragma once

#include "pass-manager.h"

namespace Helix
{
    class RegisterAllocator2 : public FunctionPass
    {
    public:
        void Execute(Function* fn, const PassRunInformation& info) override;
    };
}

REGISTER_PASS(RegisterAllocator2, regalloc2, "[ARM] V2 Register allocation (linear scan)");
