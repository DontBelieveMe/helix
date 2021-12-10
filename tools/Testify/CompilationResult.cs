namespace Testify
{
    class CompilationResult
    {
        public string CompilerStdout { get; private set; }
        public string CompilerStderr { get; private set; }

        public int CompilerExitCode { get; private set; }

        /// <summary>
        /// Milliseconds
        /// </summary>
        public long CompilationTime { get; private set; }

        public CompilationResult(string compilerStdout, string compilerStderr, int compilerExitCode, long compilationTime)
        {
            CompilerStdout = compilerStdout;
            CompilerStderr = compilerStderr;
            CompilerExitCode = compilerExitCode;
            CompilationTime = compilationTime;
        }
    }
}