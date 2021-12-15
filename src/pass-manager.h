#pragma once

#include "system.h"

#include <vector>
#include <memory>

#if defined(DECLARE_PASS_IMPL)
	#define DEFINE_PASS_LOGGER(PassName) HELIX_DEFINE_LOG_CHANNEL(PassName)
#else
	#define DEFINE_PASS_LOGGER(PassName) HELIX_EXTERN_LOG_CHANNEL(PassName)
#endif

#define REGISTER_PASS(ClassName, PassName, PassDesc) \
	namespace Helix { template <> \
	struct PassTraits<ClassName> { \
		static constexpr const char* Name = #PassName; \
		static constexpr const char* Desc = PassDesc; \
	}; } \
	DEFINE_PASS_LOGGER(PassName)

namespace Helix
{
	class Module;
	class Function;
	class BasicBlock;

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

	class BasicBlockPass : public Pass
	{
	public:
		virtual void Execute(Module* mod) override final;
		virtual void Execute(BasicBlock* bb) = 0;
	};

	template <typename T>
	struct PassTraits;

	class PassManager
	{
	public:
		PassManager();

		void Execute(Module* module);

	private:
		using CreatePassFunctor = std::unique_ptr<Pass>(*)();

		struct PassData
		{
			CreatePassFunctor create_action;
			const char* name;
			const char* desc;
		};

		template <typename T>
		void AddPass() {
			m_Passes.push_back({
				[]()-> std::unique_ptr<Pass> {
					return std::make_unique<T>();
				},

				PassTraits<T>::Name,
				PassTraits<T>::Desc
			});
		}

		std::vector<PassData> m_Passes;
	};
}