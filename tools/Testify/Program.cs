using System;
using System.Collections.Generic;

namespace Testify
{
    public class Program
    {
        private static Dictionary<string, Func<IReportPrinter>> _printers = new Dictionary<string, Func<IReportPrinter>>
        {
            // { "html", () => { return new HtmlPrinter(); } },
            { "csv-regressions", () => { return new CsvRegressionPrinter(); } }
        };

        public static Dictionary<string, Func<Testsuite>> Testsuites = new Dictionary<string, Func<Testsuite>>
        {
            { "helix",           () => { return new HelixTestsuite();       } },
            // { "ctestsuite", () => { return new CTestsuite(); } },
        };

        public static string GetLinkToTestsuite(string name)
        {
            return name + ".html";
        }

        private static void RunTestsuite(string name)
        {
            Report report;
            if (!ProgramOptions.ReportOnly)
            {
                TestRunner runner = new TestRunner(name, Testsuites[name]());
                report = runner.RunAll();

                // Update the tests JSON history "database"
                TestsDatabase.Update(report);
            }
            else
            {
                report = TestsDatabase.ReadLastReport(name);
            }

            // Always print a quick summary to the command line
            new SummaryPrinter().Print(report);

            foreach (string reportType in ProgramOptions.ReportFormats)
            {
                IReportPrinter printer = _printers[reportType]();
                printer.Print(report);
            }
        }

        static void Main(string[] args)
        {
            ProgramOptions.Parse(args);

            if (ProgramOptions.SummariseLastRun)
            {
                Report report = TestsDatabase.ReadLastReport("helix");
                new SummaryPrinter().Print(report);
                return;
            }

            if (ProgramOptions.Testsuites.Length == 0)
            {
                RunTestsuite("helix");
            }
            else
            {
                foreach (string name in ProgramOptions.Testsuites)
                {
                    RunTestsuite(name);
                }
            }
        }
    }
}
