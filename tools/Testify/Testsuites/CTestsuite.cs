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

        private string _baseDirectory;

        public CTestsuite()
        {
            string runSubdir = Path.GetRandomFileName();
            _baseDirectory = ".testify/obj/ctestsuite/" + runSubdir;

            if (!Directory.Exists(_baseDirectory))
            {
                Directory.CreateDirectory(_baseDirectory);
            }
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
                return new TestRun(TestStatus.Skipped, new CompilationResult("", "", 0, 0, filepath, "", ""), "", null);
            }

            string[] flags = { "--no-colours" };


            CompilationResult result = HelixCompiler.CompileSingleFile(_baseDirectory, filepath, string.Join(" ", flags));

            if (result.CompilerExitCode == 0)
                return new TestRun(TestStatus.Pass, result, "", null);
            else
                return new TestRun(TestStatus.Fail, result, "", null);
        }
    }
}
