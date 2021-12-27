#include "../c/frontend.h"

#include "../profile.h"
#include "../helix.h"
#include "../pass-manager.h"
#include "../helix-config.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

HELIX_DEFINE_LOG_CHANNEL(driver);

struct ProcessOutput
{
	int exit_code = 0xDEADBEE;
};

static bool ExecuteProcess(const std::string& name, const std::vector<std::string>& arguments, ProcessOutput* outProcessInfo)
{
	std::string argumentsString = "";

	for (size_t i = 0; i < arguments.size(); ++i) {
		argumentsString.append(arguments[i]);

		if (i < arguments.size() - 1) {
			argumentsString.append(" ");
		}
	}

	const std::string commandLine = fmt::format("\"{}\" {}", name, argumentsString);

	helix_info(logs::driver, "Executing process with command line '{}'", commandLine);

	STARTUPINFOA         si = {0};
	PROCESS_INFORMATION  pi = {0};

	si.cb = sizeof(si);

	// All this arsing about is nessesary because the lpCommandLine parameter of CreateProcess
	// requires a modifiable string... Documentation quoted below:
	//
	//   > The Unicode version of this function, CreateProcessW, can modify the contents of this string.
	//   > Therefore, this parameter cannot be a pointer to read-only memory (such as a const variable or a literal string).
	//   > If this parameter is a constant string, the function may cause an access violation.  
	//
	// This is just for safety, since whilst currently it's using CreateProcessA, ideally this should use
	// unicode.
	std::unique_ptr<char[]> commandLineBuffer = std::make_unique<char[]>(commandLine.size() + 1);
	memcpy(commandLineBuffer.get(), commandLine.c_str(), commandLine.size() + 1);

	if(!CreateProcessA(NULL,
	       commandLineBuffer.get(),
	       NULL,
	       NULL,
	       FALSE,
	       0,
	       NULL,
	       NULL,
	       &si,
	       &pi)
	   ) 
	{
		helix_error(logs::driver, "Failed to create process '{}'", commandLine);
		return false;
	}
	
	static constexpr DWORD dwTimeoutMilliseconds = 2000; 

	// Wait until child process exits.
	const DWORD status = WaitForSingleObject(pi.hProcess, dwTimeoutMilliseconds);

	if (status != WAIT_OBJECT_0) {
		if (status == WAIT_TIMEOUT) {
			helix_error(logs::driver, "Process timed out after {}ms", dwTimeoutMilliseconds);
		} else {
			const DWORD dwLastError = GetLastError();
			helix_error(logs::driver, "Unspecified error waiting on process (GetLastError() = 0x{0:x})", dwLastError);
		}
		
		// Error, but still make to close the process & thread handles
		// Logging error specifics is handles 
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		return false;
	}

	if (outProcessInfo) {
		DWORD exitCode = 0xDEAFBEE;
		GetExitCodeProcess(pi.hProcess, &exitCode);

		outProcessInfo->exit_code = (int) exitCode;
	}

	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return true;
}

/**
 * Parse CLI options and compile the input source file to IR.
 * `argc` & `argv` are intended to come directly from `main`
 * 
 * @param argc Number of CLI arguments.
 * @param argv Pointer to a list of the CLI option strings (argc long)
 * @return Helix::Module* Pointer to the compiled IR module, or nullptr if an error occurred
 */
static Helix::Module* ParseTranslationUnit(int argc, const char** argv)
{
	helix_trace(logs::driver, "Begin frontend compilation");

	Helix::Frontend::Initialise();
	Helix::Module* pModule = Helix::Frontend::Run(argc, argv);
	Helix::Frontend::Shutdown();

	helix_trace(logs::driver, "End frontend compilation");
	return pModule;
}

/**
 * Run the middle and backend processes on the given translation unit to
 * optimise it and finally convert it to an assembly file.
 * 
 * @param translationUnit Pointer to the translation unit/module to compile. Cannot be null.
 */
static void CompileTranslationUnit(Helix::Module* translationUnit)
{
	helix_trace(logs::driver, "Begin middle & backend compilation");

	helix_assert(translationUnit, "cannot compile null module (did frontend errors occur?)");

	Helix::PassManager passManager;
	passManager.Execute(translationUnit);

	helix_trace(logs::driver, "End middle & backend compilation");
}

/**
 * Finally take the assembly generated by the backend (see CompileTranslationUnit) and
 * assemble it (if nessesary also link it into an executable).
 * 
 * @param translationUnit Pointer to the module to assemble & link. Cannot be null.
 */
static void FinaliseExecutable(Helix::Module* translationUnit)
{
	helix_trace(logs::driver, "Begin assemble & link");
	helix_assert(translationUnit, "cannot finalise null module (did other errors occur?)");

	const std::string assemblyFileName = Helix::GetOutputFilePath(translationUnit, ".s");

	helix_info(logs::driver, "Output assembly file: {}", assemblyFileName);

	// no output path specified or printing to stdout (in which case there is no file to assemble & link)
	if (assemblyFileName.empty() || assemblyFileName == "-") {
		helix_warn(logs::driver, "Ending assemble & link, no assembly output file");
		return;
	}

	// #FIXME: A more dynamic search for GCC is probably wanted here (currently this means you can
	//         only really use the compiler on the machine that you compiled the compiler for...
	//         since the filepath to GCC is hardcoded/literally backed into the compiler executable).
#if !defined(CONFIG_GNU_TOOLS_GCC_EXE)
	const char* gccFilepath = nullptr;
#else
	const char* gccFilepath = CONFIG_GNU_TOOLS_GCC_EXE;
#endif

	if (!gccFilepath) {
		helix_error(logs::driver, "CONFIG_GNU_TOOLS_GCC_EXE is not defined, currently mandatory for finding GCC. Ending.");
		return;
	}

	helix_info(logs::driver, "Using GCC at '{}'", gccFilepath);

	std::vector<std::string> gccParameters
	{
		assemblyFileName, // First the name of the file we want to assemble

		"-nostdinc",      // Probably wont make that much of a difference, since the assembly we created
		                  // should not include any files, but it never hurts to make sure
		
		"-static"         // Statically link the LibC that comes with GCC
		                  // #FIXME: Eventually add -nostdlib here as well when we define our own standard library
	};

	if (Helix::Options::GetCompileOnly()) {
		gccParameters.push_back("-c");
	}

	ProcessOutput processOutput;

	// Finally run GCC itself!
	// As a little thought, this is actually kindof crazy - we spawn GCC as a subprocess to assemble & 
	// link for us, but GCC will itself spawn subprocess of as and ld to do the actual operations.
	// On Windows where creating processes isn't the cheapest - unlike Linux - this will hurt compilation
	// perfomance.
	//
	// #FIXME: Look at solutions for this - maybe we can utilise LLVM to do the assembly and linking
	//         in process?
	//         I seem to remember reading that clang had this very same problem when it shelled out to
	//         system tools to assemble and link.
	if (!ExecuteProcess(gccFilepath, gccParameters, &processOutput)) {
		helix_error(logs::driver, "Ending - error creating GCC subprocess, see other logs for details");
		return;
	}

	// Presume (it's pretty standard i think?) that any non zero exit code means GCC executed
	// but with errors.
	if (processOutput.exit_code != 0) {
		helix_error(logs::driver, "GCC assembly & linking failed with exit code {}", processOutput.exit_code);
		return;
	}

	// Well wasn't that quite the ride
	helix_trace(logs::driver, "End assemble & link - OK :-)");
}

int main(int argc, const char** argv)
{
	int exitCode = 0;
	Helix::Initialise();

	// Convert the input C to IR.
	Helix::Module* translationUnit = ParseTranslationUnit(argc, argv);

	if (!translationUnit) {
		exitCode = 1;
	
		helix_error(logs::driver, "Frontend error, exiting with status code {}", exitCode);
		goto end;
	}

	// Run the middle & back end passes on the translation unit and convert it to
	// assembly.
	CompileTranslationUnit(translationUnit);

	// Assemble (and if nessesary link) the assembly produced by the back end.
	FinaliseExecutable(translationUnit);

end:
	Helix::Shutdown();
	return exitCode;
}