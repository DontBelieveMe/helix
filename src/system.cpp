#include "system.h"

#include <spdlog/spdlog.h>

#include <Windows.h>

std::vector<Helix::LogRegister*> Helix::LogRegister::s_loggers;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Helix::ShouldDebugBreak()
{
	return IsDebuggerPresent();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Helix::DisableDebugLogging()
{
	//spdlog::set_level(spdlog::level::critical);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Helix::Unreachable(const char* header, int line, const char* file, const char* fn, const std::string& reason)
{
		spdlog::critical("******************** Internal Compiler Error ********************");
		spdlog::critical("    {}", header);
		spdlog::critical("");
		spdlog::critical("    File:        {}", file);
		spdlog::critical("    Line:        {}", line);
		spdlog::critical("    Function:    {}", fn);
		spdlog::critical("    Explanation: {}", reason);
		spdlog::critical("******************** Internal Compiler Error ********************");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Helix::Assert(bool cond, int line, const char* file, const char* fn, const char* condString, const std::string& reason)
{
	const bool bAssertionFailed = !cond; 

	if (bAssertionFailed) {
		spdlog::critical("******************** Internal Compiler Error ********************");
		spdlog::critical("    Assertion Failed!");
		spdlog::critical("");
		spdlog::critical("    File:      {}", file);
		spdlog::critical("    Line:      {}", line);
		spdlog::critical("    Function:  {}", fn);
		spdlog::critical("    Condition: {}", condString);
		spdlog::critical("    Reason:    {}", reason);
		spdlog::critical("******************** Internal Compiler Error ********************");
	}

	return bAssertionFailed;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
