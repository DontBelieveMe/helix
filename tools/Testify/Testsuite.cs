using System;
using System.Collections.Generic;
using System.IO;
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

        private static string ConvertWindowsToWslPath(string filepath)
        {
            // Get an absolute path so that there in no ambiguity in WSL land
            filepath = Path.GetFullPath(filepath);

            // Need to replace any Windows style path seperators (\) with unix style (/)
            filepath = filepath.Replace("\\", "/");

            // Now replace any references to drives with the WSL version
            // e.g. "C:/" becomes "/mnt/c/"
            string[] drives = Directory.GetLogicalDrives();

            foreach (string drive in drives)
            {
                string normalisedDrive = drive.Replace("\\", "/");
                string driveCharacter = drive.Substring(0, 1).ToLower();

                string inputFilepath = filepath;
                filepath = filepath.Replace(normalisedDrive, "/mnt/" + driveCharacter + "/");

                // There can only be one drive reference in a path so
                // if we've already replaced it then don't bother trying to replace any others
                if (filepath != inputFilepath)
                {
                    break;
                }
            }

            return filepath;
        }

        public static ProgramOutput RunCompiledFile(CompilationResult compilation)
        {
            // Get a path to the compiled executable in WSL land
            string wslExecutablePath = ConvertWindowsToWslPath(compilation.OutputExecutableFilepath);
            
            // Finally execute the program using WSL. The return code will be the return code of the
            // executed program.

            string   program = "wsl";

            // Utility script run-arm.sh will invoke the QEMU arm userspace emulator to execute the
            // given program.
            string[] args =
            {
                "tools/run-arm.sh", // First argument to 'wsl' is the program to execute (in this case our script)
                wslExecutablePath,  // Now it's parameters to pass to the first program. Here it's the compiled program to execute.
                "-quiet"            // Tells the script not to print a message with the exit code to stdout
            };

            return ProcessHelpers.RunExternalProcess(program, args);
        }
    }

    public abstract class Testsuite
    {
        public abstract TestRun RunTest(string filepath);
        public abstract string[] GetAllTestFiles();
    }
}
