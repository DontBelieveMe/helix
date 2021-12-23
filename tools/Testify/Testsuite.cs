using System;
using System.Collections.Generic;
using System.Text;

namespace Testify
{
    public enum TestStatus
    {
        Pass,
        Fail,
        XFail,
        Skipped
    }

    public class TestRun
    {
        public TestStatus Status { get; private set; }
        public CompilationResult Compilation { get; private set; }
        public string ExpectedStdout { get; private set; }
        public ProgramOutput ExecutionResult { get; private set; }

        public TestRun(TestStatus status, CompilationResult compilation, string expectedStdout, ProgramOutput executionResult)
        {
            this.Status = status;
            this.Compilation = compilation;
            ExpectedStdout = expectedStdout;
            ExecutionResult = executionResult;
        }
    }

    class CommonTestsuiteActions
    {
        public static TestRun FailedTest(TestStatus expectedStatus, CompilationResult result, string expectedStdout, ProgramOutput execOutput = null)
        {
            // If we expected a test to fail (or expected it to be an xfail) and it did fail, then report
            // it as an xfail instead of an actual failure
            if (expectedStatus == TestStatus.Fail || expectedStatus == TestStatus.XFail)
                return new TestRun(TestStatus.XFail, result, expectedStdout, execOutput);

            return new TestRun(TestStatus.Fail, result, expectedStdout, execOutput);
        }

        private static string MakeFilepathWSLFriendly(string filepath)
        {
            filepath = filepath.Replace("\\", "/");
            return filepath.Replace("C:", "/mnt/c");
        }

        public static ProgramOutput RunCompiledFile(CompilationResult compilation)
        {
            string wslExecutablePath = MakeFilepathWSLFriendly(compilation.OutputExecutableFilepath);
            string program = "bash";
            string[] args = { "tools/run-arm.sh", wslExecutablePath };

            return ProcessHelpers.RunExternalProcess(program, string.Join(" ", args));
        }
    }

    public interface ITestsuite
    {
        TestRun RunTest(string filepath);
        string[] GetAllTestFiles();
    }
}
