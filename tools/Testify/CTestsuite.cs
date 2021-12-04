using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;

namespace Testify
{
    class CTestsuite
    {
        public void Run()
        {
            Console.WriteLine("Running c-testsuite tests (https://github.com/c-testsuite/c-testsuite)...");

            if (!Directory.Exists("extras/c-testsuite"))
            {
                Console.Write("[");
                Console.ForegroundColor = ConsoleColor.Red;
                Console.Write("error");
                Console.ResetColor();
                Console.Write("] extras/c-testsuite does not exist?");
                return;
            } else
            {
                Console.Write("Found extras/c-testsuite directory...");
            }

            TestsuiteStats stats = new TestsuiteStats();
            Stopwatch timer = Stopwatch.StartNew();

            foreach (string file in Directory.GetFiles("extras/c-testsuite/tests/single-exec/", "*.c", SearchOption.AllDirectories))
            {
                string[] args = { file, "--" };

                Stopwatch stopwatch = Stopwatch.StartNew();
                ProgramOutput output = ProcessHelpers.RunExternalProcess("vs2019/Debug/helix.exe", string.Join(" ", args));
                stopwatch.Stop();
                stats.ExecutionTimes.Add(stopwatch.ElapsedMilliseconds);

                Console.Write(file + ".. .");

                if (output.ExitCode == 0)
                {
                    Console.Write("[");
                    Console.ForegroundColor = ConsoleColor.Green;
                    Console.Write("pass");
                    Console.ResetColor();
                    Console.WriteLine("]");
                    stats.Passes += 1;
                }
                else
                {
                    Console.Write("[");
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.Write("fail");
                    Console.ResetColor();
                    Console.WriteLine("]");
                    stats.Fails += 1;
                }
            }

            Console.WriteLine();
            Console.WriteLine("[Summary]");

            Console.Write("\t");
            Console.ForegroundColor = ConsoleColor.Green;
            Console.Write("PASSES");
            Console.ResetColor();
            Console.WriteLine(": {0} ({1:0.00}%)", stats.Passes, stats.CalculatePassPercentage());

            Console.Write("\t");
            Console.ForegroundColor = ConsoleColor.Red;
            Console.Write("FAILS");
            Console.ResetColor();
            Console.WriteLine(":  {0} ({1:0.00}%)", stats.Fails, stats.CalculateFailPercentage());

            Console.Write("\t");
            Console.ForegroundColor = ConsoleColor.DarkYellow;
            Console.Write("XFAILS");
            Console.ResetColor();
            Console.WriteLine(": {0} ({1:0.00}%)", stats.XFails, stats.CalculateXFailPercentage());

            Console.WriteLine();
            Console.WriteLine("Executing {0} tests took {1:0.00}s (average of {4:0.00}ms each, range {2}ms -> {3}ms)",
                stats.TotalRuns, timer.ElapsedMilliseconds / 1000.0, stats.GetSmallestExecutionTime(), stats.GetLargestExecutionTime(), stats.GetAverageExecutionTime());
            Console.WriteLine();

        }
    }
}
