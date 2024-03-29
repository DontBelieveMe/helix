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

HELIX_EXTERN_LOG_CHANNEL(pass_manager);

namespace Helix
{
	class Module;
	class Function;
	class BasicBlock;
	class ValidationPass;

	struct PassRunInformation
	{
		bool TestTrace = false;
	};

	class Pass
	{
	public:
		virtual ~Pass() = default;

		virtual void Execute(Module* mod, const PassRunInformation& info) = 0;
	};

	class FunctionPass : public Pass
	{
	public:
		virtual void Execute(Module* mod, const PassRunInformation& info) override final;
		virtual void Execute(Function* fn, const PassRunInformation& info) = 0;
	};

	class BasicBlockPass : public Pass
	{
	public:
		virtual void Execute(Module* mod, const PassRunInformation& info) override final;
		virtual void Execute(BasicBlock* bb, const PassRunInformation& info) = 0;
	};

	template <typename T>
	struct PassTraits;

	class PassManager
	{
		struct PassData;

	public:
		PassManager();

		void Execute(Module* module);

	private:
		void ValidateModule(ValidationPass& validationPass, Module* module);
		void RunPass(const PassData& passData, Module* module);

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

			helix_trace(logs::pass_manager, "Registered pass ({}) '{}' - {}", m_Passes.size(), PassTraits<T>::Name, PassTraits<T>::Desc);
		}

		std::vector<PassData> m_Passes;
	};
}