using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace Testify
{
    class CsvRegressionPrinter : IReportPrinter
    {
        public void Print(Report report)
        {
            List<Summary> summaries = TestsDatabase.ReadSummaryHistory(report.Name);

            StringBuilder csv = new StringBuilder();

            csv.AppendLine("When, Passes, Fails, Skipped, Total");

            foreach (Summary summary in summaries)
            {
                csv.AppendLine(string.Format("{0}, {1}, {2}, {3}, {4}", summary.When, summary.Passes, summary.Fails, summary.Skipped, summary.Total));
            }

            File.WriteAllText(".testify/" + report.Name + ".csv", csv.ToString());
        }
    }
}
