///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// File: system.h
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Defines core/system level utilites for example:
//    Custom assertion macros (helix_assert, helix_unreachable)
//    Logging
//    General utility macros such as HELIX_NO_STEAL
//
// This header gets included about _alot_, so try to avoid including other headers (especially big ones) here.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "profile.h"

#define HELIX_DEBUG_BREAK (Helix::ShouldDebugBreak() ? __debugbreak() : exit(1))

#define helix_assert(cond, reason) \
	do { \
		if (!(cond)) { if(Helix::Assert(cond, __LINE__, __FILE__, __FUNCTION__, #cond, reason)) { \
			HELIX_DEBUG_BREAK; \
		}} \
	} while(0)

#define helix_unreachable(desc) \
	do { \
		Helix::Unreachable("Unreachable code reached (paradoxical, i know)", __LINE__, __FILE__, __FUNCTION__, desc); \
		HELIX_DEBUG_BREAK; \
	} while(0)

#define helix_unimplemented(desc) \
	do { \
		Helix::Unreachable("Not implemented", __LINE__, __FILE__, __FUNCTION__, desc); \
		HELIX_DEBUG_BREAK; \
	} while(0)

#define HELIX_DEFINE_LOG_CHANNEL(name) \
	namespace logs { \
		spdlog::logger name(#name); \
		namespace defs { \
			static std::shared_ptr<spdlog::logger> ptr##name { std::shared_ptr<spdlog::logger>{}, &::logs::name }; \
			static Helix::LogRegister reg##name(#name, ptr##name); \
		} \
	}

#define HELIX_EXTERN_LOG_CHANNEL(name) namespace logs { extern spdlog::logger name; }

#define helix_trace(channel, ...) channel##.trace(__VA_ARGS__)
#define helix_info(channel, ...) channel##.info(__VA_ARGS__)
#define helix_warn(channel, ...) channel##.warn(__VA_ARGS__)
#define helix_debug(channel, ...) channel##.debug(__VA_ARGS__)

HELIX_EXTERN_LOG_CHANNEL(general);

/// Delete the copy constructor, move constructor, copy assignment operator
/// and move assignment operator for the class 'ClassName'. Use inside the
/// given class, for example:
///
///   class MyString {
///   public:
///       HELIX_NO_STEAL(MyString);
///   };
///
#define HELIX_NO_STEAL(ClassName) \
	ClassName(const ClassName&) = delete; \
	ClassName(ClassName&&) = delete; \
	ClassName& operator=(const ClassName&) = delete; \
	ClassName& operator=(ClassName&&) = delete

namespace Helix
{
	// #FIXME(bwilks): Move most of this to the implementation file.
	struct LogRegister
	{
		const char*     name;
		std::shared_ptr<spdlog::logger> plogger;

		LogRegister(const char* name, const std::shared_ptr<spdlog::logger>& plogger)
			: name(name), plogger(plogger)
			{
				LogRegister::s_loggers.push_back(this);
			}

		static void init_all()
		{
			spdlog::sink_ptr stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

			for (LogRegister* logger : s_loggers) {
				logger->plogger->sinks().push_back(stdout_sink);
				logger->plogger->set_level(spdlog::level::critical);
				logger->plogger->flush_on(spdlog::level::trace);

				spdlog::register_logger(logger->plogger);
			}
		}

		static void set_all_log_levels(spdlog::level::level_enum level) {
			spdlog::set_level(level);
		}

		static void set_log_level(const char* name, spdlog::level::level_enum level)
		{
			auto logger = spdlog::get(name);

			if (logger) {
				logger->set_level(level);
			}
		}

	private:
		static std::vector<LogRegister*> s_loggers;
	};

	bool ShouldDebugBreak();

	/// Disable any debug logging from this point onwards (e.g. logs made with helix_[trace,info,warn] etc...)
	void DisableDebugLogging();

	/// Internal function, used by helix_unreachable - Don't use this function directly!
	void Unreachable(const char* header, int line, const char* file, const char* fn, const std::string& reason);

	/// Internal function, used by helix_assert - Don't use this function directly!
	bool Assert(bool cond, int line, const char* file, const char* fn, const char* condString, const std::string& reason);
}