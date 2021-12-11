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

        public TestRun(TestStatus status, CompilationResult compilation, string expectedStdout)
        {
            this.Status = status;
            this.Compilation = compilation;
            ExpectedStdout = expectedStdout;
        }
    }

    class CommonTestsuiteActions
    {
        public static TestRun FailedTest(TestStatus expectedStatus, CompilationResult result, string expectedStdout)
        {
            // If we expected a test to fail (or expected it to be an xfail) and it did fail, then report
            // it as an xfail instead of an actual failure
            if (expectedStatus == TestStatus.Fail || expectedStatus == TestStatus.XFail)
                return new TestRun(TestStatus.XFail, result, expectedStdout);

            return new TestRun(TestStatus.Fail, result, expectedStdout);
        }
    }

    public interface ITestsuite
    {
        TestRun RunTest(string filepath);
        string[] GetAllTestFiles();
    }
}
