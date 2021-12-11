using System;
using System.Collections.Generic;
using System.Text;

namespace Testify
{
    class StubTestsuite : ITestsuite
    {
        public string[] GetAllTestFiles()
        {
            return new string[] { "foo.c", "bar.c" };
        }

        public TestRun RunTest(string filepath)
        {
            return new TestRun(TestStatus.Skipped, new CompilationResult("", "", 0, 10, filepath));
        }
    }
}
