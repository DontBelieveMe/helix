﻿using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Text;

namespace Testify
{
    class TestRunner
    {
        private Testsuite _testsuite;
        private string _name;

        public TestRunner(string name, Testsuite testsuite)
        {
            _name = name;
            _testsuite = testsuite;
        }

        private string GetStatusAsString(TestStatus status)
        {
            switch (status)
            {
                case TestStatus.Fail: return "fail";
                case TestStatus.Pass: return "pass";
                case TestStatus.Skipped: return "skipped";
                case TestStatus.XFail: return "xfail";
            }

            return "";
        }

        private ConsoleColor GetStatusColour(TestStatus status)
        {
            switch (status)
            {
                case TestStatus.Fail: return ConsoleColor.Red;
                case TestStatus.Pass: return ConsoleColor.Green;
                case TestStatus.Skipped: return ConsoleColor.Cyan;
                case TestStatus.XFail: return ConsoleColor.DarkYellow;
            }

            return ConsoleColor.White;
        }

        public Report RunAll()
        {
            string[] files = _testsuite.GetAllTestFiles();

            Report report = new Report();

            Stopwatch timer = Stopwatch.StartNew();

            if (_testsuite.GetOverrideAbortEarly())
                report.AbortedEarly = true;

            foreach (string file in files)
            {
                Console.Write("{0}... ", file);
                TestRun testInfo = _testsuite.RunTest(file);
                report.AddTest(testInfo);

                Console.Write("[");
                Console.ForegroundColor = GetStatusColour(testInfo.Status);
                Console.Write(GetStatusAsString(testInfo.Status));
                Console.ResetColor();
                Console.WriteLine("]");

                if (ProgramOptions.Verbose)
                {
                    Console.WriteLine("> {0}", testInfo.Compilation.CompilerCommandLine);
                }

                if (testInfo.Status == TestStatus.Fail && ProgramOptions.DumpDiffs)
                {
                    Console.WriteLine("************************** ACTUAL STDOUT ****************************");
                    Console.WriteLine(testInfo.Compilation.CompilerStdout);
                    Console.WriteLine("************************** EXPECTED STDOUT **************************");
                    Console.WriteLine(testInfo.ExpectedStdout);
                    Console.WriteLine("*********************************************************************");
                }

                if (ProgramOptions.FailFast && testInfo.Status == TestStatus.Fail)
                {
                    report.AbortedEarly = true;
                    break;
                }
            }

            timer.Stop();

            report.Name = _name;
            report.OverallRuntime = timer.ElapsedMilliseconds;
            report.GenerationTime = DateTime.Now;

            return report;
        }
    }
}
