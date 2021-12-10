using System;
using System.Collections.Generic;
using System.Text;

namespace Testify
{
    class SummaryPrinter : IReportPrinter
    {
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
            Console.WriteLine("Executing {0} tests took {1:0.00}s (average of {4:0.00}ms each, range {2}ms -> {3}ms)",
                totalRuns, totalTimeSeconds, smallest, largest, average
            );
            Console.WriteLine();
        }
    }
}
