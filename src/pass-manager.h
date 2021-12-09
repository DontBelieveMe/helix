#pragma once

#include <vector>
#include <memory>

namespace Helix
{
    class Module;
    class Function;

    class Pass
    {
    public:
        virtual ~Pass() = default;

        virtual void Execute(Module* mod) = 0;
    };

    class FunctionPass : public Pass
    {
    public:
        virtual void Execute(Module* mod) override final;
        virtual void Execute(Function* fn) = 0;
    };

    class PassManager
    {
    public:
        PassManager();

        void Execute(Module* module);

    private:
        using CreatePassFunctor = std::unique_ptr<Pass>(*)();

        template <typename T>
        void AddPass() {
            m_Passes.emplace_back([]()-> std::unique_ptr<Pass> {
                return std::make_unique<T>();
            });
        }

        std::vector<CreatePassFunctor> m_Passes;
    };
}