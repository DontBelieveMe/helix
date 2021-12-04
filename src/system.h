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

#include "profile.h"

#define HELIX_DEBUG_BREAK (Helix::ShouldDebugBreak() ? __debugbreak() : exit(1))

#define helix_assert(cond, reason) \
	do { \
		if (cond && Helix::Assert(cond, __LINE__, __FILE__, __FUNCTION__, #cond, reason)) { \
			HELIX_DEBUG_BREAK; \
		} \
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

#define helix_trace(...) SPDLOG_TRACE(__VA_ARGS__)
#define helix_info(...) SPDLOG_INFO(__VA_ARGS__)
#define helix_warn(...) SPDLOG_WARN(__VA_ARGS__)

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
	bool ShouldDebugBreak();

	/// Disable any debug logging from this point onwards (e.g. logs made with helix_[trace,info,warn] etc...)
	void DisableDebugLogging();

	/// Internal function, used by helix_unreachable - Don't use this function directly!
	void Unreachable(const char* header, int line, const char* file, const char* fn, const std::string& reason);

	/// Internal function, used by helix_assert - Don't use this function directly!
	bool Assert(bool cond, int line, const char* file, const char* fn, const char* condString, const std::string& reason);
}