using System.Collections.Generic;

namespace Testify
{
    public class CompilationResult
    {
        public string CompilerStdout { get; private set; }
        public string CompilerStderr { get; private set; }
        public string SourceFile { get; private set; }

        public int CompilerExitCode { get; private set; }

        public string AssemblyOutput { get; private set; }

        /// <summary>
        /// Milliseconds
        /// </summary>
        public long CompilationTime { get; private set; }

        public List<string> CompilationCommands { get; private set; } = new List<string>();

        public string OutputExecutableFilepath { get; private set; }

        public CompilationResult(string compilerStdout, string compilerStderr, int compilerExitCode, long compilationTime, string sourceFile, string assemblyOutput, string outputExecutableFilepath)
        {
            CompilerStdout = compilerStdout;
            CompilerStderr = compilerStderr;
            CompilerExitCode = compilerExitCode;
            CompilationTime = compilationTime;
            SourceFile = sourceFile;
            AssemblyOutput = assemblyOutput;
            OutputExecutableFilepath = outputExecutableFilepath;
        }

        public void AppendStdout(string stdout)
        {
            CompilerStdout += stdout;
        }

        public void SetExitCode(int exitCode)
        {
            CompilerExitCode = exitCode;
        }

        public void AppendStderr(string stderr)
        {
            CompilerStderr += stderr;
        }

        public static readonly CompilationResult SkippedCompilation = new CompilationResult("", "", 0, 0, "", "","");
    }
}