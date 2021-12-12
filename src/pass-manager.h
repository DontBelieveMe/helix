#pragma once

#include <vector>
#include <memory>

#define REGISTER_PASS(ClassName, PassName, PassDesc) \
	template <> \
	struct PassTraits<ClassName> { \
		static constexpr const char* Name = PassName; \
		static constexpr const char* Desc = PassDesc; \
	}

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