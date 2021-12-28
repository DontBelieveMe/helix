using System.Collections.Generic;

namespace Testify
{
    public class CompilationResult
    {
        /// <summary>
        /// Text printed to stdout by the compiler.
        /// </summary>
        public string CompilerStdout { get; private set; }

        /// <summary>
        /// Text printed to stderr by the compiler.
        /// </summary>
        public string CompilerStderr { get; private set; }

        /// <summary>
        /// Exit code of the compiler.
        /// </summary>
        public int    CompilerExitCode { get; private set; }

        /// <summary>
        /// Path to the input source file that was compiled.
        /// </summary>
        public string SourceFile { get; private set; }

        /// <summary>
        /// Path to the executable created by the compilation.
        /// </summary>
        public string OutputExecutableFilepath { get; private set; }

        /// <summary>
        /// Time for the compilation (in milliseconds).
        /// </summary>
        public long CompilationTime { get; private set; }

        /// <summary>
        /// Command line used to compile the input source file.
        /// </summary>
        public string CompilerCommandLine { get; private set; }

        public CompilationResult(string compilerStdout, string compilerStderr, int compilerExitCode, string sourceFile,
            string outputExecutableFilepath, long compilationTime, string compilerCommandLine)
        {
            CompilerStdout = compilerStdout;
            CompilerStderr = compilerStderr;
            CompilerExitCode = compilerExitCode;
            SourceFile = sourceFile;
            OutputExecutableFilepath = outputExecutableFilepath;
            CompilationTime = compilationTime;
            CompilerCommandLine = compilerCommandLine;
        }

        public static CompilationResult CreateSkippedCompilation(string inputSourcefile)
        {
            return new CompilationResult("", "", 0, inputSourcefile, "", 0, "");
        }
    }
}