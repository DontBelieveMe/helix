using System;
using System.Collections.Generic;
using System.Text;

namespace Testify
{
    class SummaryPrinter : IReportPrinter
    {
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

        private void PrintStatusStat(Report report, ConsoleColor colour, string name, TestStatus status)
        {
            int total = report.GetTotalWithStatus(status);
            int overallTotal = report.TotalTestsRan;
            double percentage = (total / (double)overallTotal) * 100.0;

            Console.Write("\t");
            Console.ForegroundColor = colour;
            Console.Write(name);
            Console.ResetColor();
            Console.WriteLine(": {0} ({1:0.00}%)", total, percentage);
        }

        public void Print(Report report)
        {
            if (ProgramOptions.SummaryMode == SummaryMode.Long)
            {
                foreach (TestRun run in report.Tests)
                {
                    Console.Write("{0}... ", run.Compilation.SourceFile);
                    Console.Write("[");
                    Console.ForegroundColor = GetStatusColour(run.Status);
                    Console.Write(GetStatusAsString(run.Status));
                    Console.ResetColor();
                    Console.WriteLine("]");
                }
            }

            Console.WriteLine();
            Console.WriteLine("[Summary]");

            PrintStatusStat(report, ConsoleColor.Green, "PASSES", TestStatus.Pass);
            PrintStatusStat(report, ConsoleColor.Red, "FAILS", TestStatus.Fail);
            PrintStatusStat(report, ConsoleColor.DarkYellow, "XFAILS", TestStatus.XFail);
            PrintStatusStat(report, ConsoleColor.Cyan, "SKIPPED", TestStatus.Skipped);

            int totalRuns = report.TotalTestsRan;

            long smallest = report.GetSmallestCompilationTime();
            long largest = report.GetLargestCompilationTime();
            double average = report.GetAverageCompilationTime();

            double totalTimeSeconds = report.OverallRuntime / 1000.0;

            Console.WriteLine();
            Console.WriteLine("Testsuite run at {0}", report.GenerationTime.ToString());
            Console.WriteLine("Executing {0} tests took {1:0.00}s (average of {4:0.00}ms each, range {2}ms -> {3}ms)",
                totalRuns, totalTimeSeconds, smallest, largest, average
            );
            Console.WriteLine();
        }
    }
}
