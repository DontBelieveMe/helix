using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;

namespace Testify
{
    class HelixTestsuite : ITestsuite
    {
        public string[] GetAllTestFiles()
        {
            return Directory.GetFiles("testsuite", "*.xml", SearchOption.AllDirectories);
        }

        private string _baseDirectory;

        public HelixTestsuite()
        {
            string runSubdir = Path.GetRandomFileName();
            _baseDirectory = ".testify/obj/helix/" + runSubdir;

            if (!Directory.Exists(_baseDirectory))
            {
                Directory.CreateDirectory(_baseDirectory);
            }
        }

        private bool TextMatches(string expected, string actual, TextCompareMode mode)
        {
            if (mode == TextCompareMode.PlainText)
            {
                return expected == actual;
            }
            else if (mode == TextCompareMode.Regex)
            {
                return Regex.IsMatch(actual, expected);
            }

            return false;
        }

        public TestRun RunTest(string filepath)
        {
            string sourcefilePath = filepath.Replace(".xml", ".c");
            StandardTestDefinition testDefinition = StandardTestDefinition.FromXml(filepath);

            string[] flags = { "--no-colours", testDefinition.CompilationFlags };

            CompilationResult result = HelixCompiler.CompileSingleFile(_baseDirectory, sourcefilePath, string.Join(" ", flags));

            string expectedStdout = testDefinition.ExpectedOutput.Trim().Replace("\r\n", "\n");
            string actualStdout = result.CompilerStdout.Trim().Replace("\r\n", "\n");

            if (result.CompilerExitCode != 0)
            {
                return CommonTestsuiteActions.FailedTest(testDefinition.ExpectedStatus, result, expectedStdout);
            }

            if (!TextMatches(expectedStdout, actualStdout, testDefinition.ExpectedOutputComparisonMode))
            {
                return CommonTestsuiteActions.FailedTest(testDefinition.ExpectedStatus, result, expectedStdout);
            }

            ProgramOutput execOutput = CommonTestsuiteActions.RunCompiledFile(result);

            if (execOutput.ExitCode != 0)
            {
                return CommonTestsuiteActions.FailedTest(testDefinition.ExpectedStatus, result, expectedStdout);
            }

            return new TestRun(TestStatus.Pass, result, expectedStdout, execOutput);
        }
    }
}
