namespace Testify
{
    public class CompilationResult
    {
        public string CompilerStdout { get; private set; }
        public string CompilerStderr { get; private set; }
        public string SourceFile { get; private set; }

        public int CompilerExitCode { get; private set; }

        /// <summary>
        /// Milliseconds
        /// </summary>
        public long CompilationTime { get; private set; }

        public CompilationResult(string compilerStdout, string compilerStderr, int compilerExitCode, long compilationTime, string sourceFile)
        {
            CompilerStdout = compilerStdout;
            CompilerStderr = compilerStderr;
            CompilerExitCode = compilerExitCode;
            CompilationTime = compilationTime;
            SourceFile = sourceFile;
        }

        public static readonly CompilationResult SkippedCompilation = new CompilationResult("", "", 0, 0, "");
    }
}