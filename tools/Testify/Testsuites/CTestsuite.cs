using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;

using Newtonsoft.Json;

namespace Testify
{
    class CTestsuite : ITestsuite
    {
        public string[] GetAllTestFiles()
        {
            return Directory.GetFiles("extras/c-testsuite/tests/single-exec", "*.c", SearchOption.AllDirectories);
        }

        public TestRun RunTest(string filepath)
        {
            string tagsFilepath = filepath + ".tags";
            string[] tags = File.ReadAllLines(tagsFilepath);

            bool skip = false;

            if (tags.Contains("needs-libc"))
                skip = true;

            if (skip)
            {
                return new TestRun(TestStatus.Skipped, CompilationResult.SkippedCompilation);
            }

            CompilationResult result = HelixCompiler.CompileSingleFile(filepath, "");

            if (result.CompilerExitCode == 0)
                return new TestRun(TestStatus.Pass, result);
            else
                return new TestRun(TestStatus.Fail, result);
        }
    }
}
