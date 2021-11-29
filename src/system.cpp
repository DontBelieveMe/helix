#include "system.h"

#include <spdlog/spdlog.h>

#include <Windows.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Helix::ShouldDebugBreak()
{
	return IsDebuggerPresent();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Helix::DisableDebugLogging()
{
	spdlog::set_level(spdlog::level::critical);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Helix::Unreachable(int line, const char* file, const char* reason)
{
		spdlog::critical("******************** Internal Compiler Error ********************");
		spdlog::critical("    Unreachable code reached (paradoxical, i know)");
		spdlog::critical("");
		spdlog::critical("    File:        {}", file);
		spdlog::critical("    Line:        {}", line);
		spdlog::critical("    Explanation: {}", reason);
		spdlog::critical("******************** Internal Compiler Error ********************");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Helix::Assert(bool cond, int line, const char* file, const char* condString, const char* reason)
{
	const bool bAssertionFailed = !cond; 

	if (bAssertionFailed) {
		spdlog::critical("******************** Internal Compiler Error ********************");
		spdlog::critical("    Assertion Failed!");
		spdlog::critical("");
		spdlog::critical("    File:      {}", file);
		spdlog::critical("    Line:      {}", line);
		spdlog::critical("    Condition: {}", condString);
		spdlog::critical("    Reason:    {}", reason);
		spdlog::critical("******************** Internal Compiler Error ********************");
	}

	return bAssertionFailed;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
