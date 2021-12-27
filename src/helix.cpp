#include "helix.h"
#include "types.h"
#include "options.h"
#include "system.h"
#include "target-info-armv7.h"

#pragma optimize("", off)

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)

#include <Windows.h>

int CRTReportCallback( int reportType, char *message, int *returnValue )
{
	(void) returnValue;

	const char* reportTypeString = [reportType]() -> const char* {
		switch (reportType) {
		case _CRT_WARN:   return "_CRT_WARN";
		case _CRT_ERROR:  return "_CRT_ERROR";
		case _CRT_ASSERT: return "_CRT_ASSERT";
		default:
			return "Unknown Kind";
		}
	}();

	helix_unreachable(fmt::format("CRT Report ({} [{}]): {}", reportTypeString, reportType, message));

	return 1;
}

#endif

void Helix::Initialise()
{
	Helix::LogRegister::init_all();
	if (Helix::Options::GetDisableLogging()) {
		Helix::DisableDebugLogging();
	}

#if defined(_MSC_VER)
	_CrtSetReportHook(CRTReportCallback);
#endif

	BuiltinTypes::Init();
	PhysicalRegisters::Init();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Helix::Shutdown()
{
	BuiltinTypes::Destroy();
}

/*********************************************************************************************************************/

static std::string NormalisePath(const std::string& Path)
{
	std::string Result = Path;

	for (char& Char : Result)
	{
		if (Char == '\\')
		{
			Char = '/';
		}
	}

	return Result;
}

/*********************************************************************************************************************/

static std::string GetFilenameWithoutExtension(const std::string& path)
{
	if (path.empty()) {
		return {};
	}

	const std::string NormalisedPath = NormalisePath(path);

	int LastSeperatorIndex = -1;
	int LastDotIndex = -1;

	for (size_t Index = 0; Index < NormalisedPath.length(); ++Index)
	{
		if (NormalisedPath[Index] == '/')
		{
			LastSeperatorIndex = (int) Index;
		}

		if (NormalisedPath[Index] == '.')
		{
			LastDotIndex = (int) Index;
		}
	}

	// Move the seperator index so it points at the character after the '/' and not the '/' itself.
	//
	// Always doing this also ensures that for filepaths without any seperators that the index
	// will point to the start of the string (since for paths without any seperators then we presume
	// the whole string is a filename), since LastSeperatorIndex will leave the above loop as -1
	// and -1 + 1 = 0.
	LastSeperatorIndex++;

	if (LastSeperatorIndex >= (int) NormalisedPath.length())
	{
		return {};
	}

	// '/' after '.' means that the '.' wasn't an extension dot but part of a filepath
	if (LastDotIndex < 0 || LastSeperatorIndex > LastDotIndex)
	{
		return NormalisedPath.substr(LastSeperatorIndex);
	}

	const int filenameLength = LastDotIndex - LastSeperatorIndex;

	return NormalisedPath.substr(LastSeperatorIndex, filenameLength);
}


/*********************************************************************************************************************/

std::string Helix::GetOutputFilePath(Module* module, const char* suffix)
{
	const std::string& userDefinedOutputFile = Options::GetOutputFile();

	if (!userDefinedOutputFile.empty())
		return userDefinedOutputFile;

	const std::string fileName = GetFilenameWithoutExtension(module->GetInputSourceFile());

	return fmt::format("{}{}", fileName, suffix);
}

std::string Helix::GetAssemblyOutputFilePath(Module* module)
{
	if (Options::GetOnlyDumpAssembly()) {
		return Helix::GetOutputFilePath(module, ".s");
	}

	const std::string fileName = GetFilenameWithoutExtension(module->GetInputSourceFile());
	return fmt::format("{}.s", fileName);
}
