#include "helix.h"
#include "types.h"
#include "options.h"
#include "system.h"
#include "target-info-armv7.h"

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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
