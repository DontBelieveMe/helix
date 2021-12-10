using System;
using System.Collections.Generic;
using System.Text;

namespace Testify
{
    enum TestStatus
    {
        Pass,
        Fail,
        XFail,
        Skipped
    }

    class TestRun
    {
        public TestStatus Status { get; private set; }
        public long CompilationTime { get; private set; }

        public TestRun(TestStatus status, long compilationTime)
        {
            this.Status = status;
            this.CompilationTime = compilationTime;
        }
    }

    class CommonTestsuiteActions
    {
        public static TestRun FailedTest(TestStatus expectedStatus, CompilationResult result)
        {
            // If we expected a test to fail (or expected it to be an xfail) and it did fail, then report
            // it as an xfail instead of an actual failure
            if (expectedStatus == TestStatus.Fail || expectedStatus == TestStatus.XFail)
                return new TestRun(TestStatus.XFail, result.CompilationTime);

            return new TestRun(TestStatus.Fail, result.CompilationTime);
        }
    }

    interface ITestsuite
    {
        TestRun RunTest(string filepath);
        string[] GetAllTestFiles();
    }
}
