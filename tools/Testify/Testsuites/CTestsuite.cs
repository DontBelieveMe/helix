using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;

using Newtonsoft.Json;

namespace Testify
{
    class CTestsuite : Testsuite
    {
        public override string[] GetAllTestFiles()
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

        public override TestRun RunTest(string filepath)
        {
            string tagsFilepath = filepath + ".tags";
            string[] tags = File.ReadAllLines(tagsFilepath);

            bool skip = false;

            if (tags.Contains("needs-libc"))
                skip = true;

            if (skip)
            {
                return new TestRun(TestStatus.Skipped, CompilationResult.CreateSkippedCompilation(filepath), "", null);
            }

            string outputExecutableFilepath = _baseDirectory + Path.GetFileNameWithoutExtension(filepath);
            CompilationResult result = HelixCompiler.CompileSingleFile(filepath, new string[] { }, outputExecutableFilepath);

            if (result.CompilerExitCode != 0)
                return CommonTestsuiteActions.FailedTest(TestStatus.Pass, result, "");

            ProgramOutput execOutput = CommonTestsuiteActions.RunCompiledFile(result);

            if (execOutput.ExitCode != 0)
                return CommonTestsuiteActions.FailedTest(TestStatus.Pass, result, "", execOutput);

            return new TestRun(TestStatus.Pass, result, "", null);
        }
    }
}
