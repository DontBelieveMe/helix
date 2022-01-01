using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;

namespace Testify
{
    class HelixTestsuite : Testsuite
    {
        public override string[] GetAllTestFiles()
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

        public override TestRun RunTest(string filepath)
        {
            // The source file to test should have the same name as the XML file (just with a .c)
            // extension
            string sourcefilePath = filepath.Replace(".xml", ".c");

            // Parse the test definition, in the standard test definition XML format.
            StandardTestDefinition testDefinition = StandardTestDefinition.FromXml(filepath);

            string[] extraCompilationFlags = { testDefinition.CompilationFlags };

            string outputExecutableFilepath = _baseDirectory + "/" + Path.GetFileNameWithoutExtension(sourcefilePath);

            CompilationResult result = HelixCompiler.CompileSingleFile(sourcefilePath, extraCompilationFlags, outputExecutableFilepath);

            string expectedStdout = testDefinition.ExpectedOutput != null ? testDefinition.ExpectedOutput.Trim().Replace("\r\n", "\n") : "";

            if (result.CompilerExitCode != 0)
            {
                return CommonTestsuiteActions.FailedTest(testDefinition.ExpectedStatus, result, expectedStdout);
            }

            if (testDefinition.ExpectedOutput != null)
            {
                string actualStdout = result.CompilerStdout.Trim().Replace("\r\n", "\n");

                if (!TextMatches(expectedStdout, actualStdout, testDefinition.ExpectedOutputComparisonMode))
                {
                    return CommonTestsuiteActions.FailedTest(testDefinition.ExpectedStatus, result, expectedStdout);
                }
            }

            ProgramOutput execOutput = null;

            if (!testDefinition.CompilationFlags.Contains("-c") &&
                !testDefinition.CompilationFlags.Contains("-S"))
            {
                execOutput = CommonTestsuiteActions.RunCompiledFile(result);

                if (execOutput.ExitCode != testDefinition.ExecutableExpectedExitCode)
                {
                    return CommonTestsuiteActions.FailedTest(testDefinition.ExpectedStatus, result, expectedStdout);
                }
            }

            return new TestRun(TestStatus.Pass, result, expectedStdout, execOutput);
        }
    }
}
