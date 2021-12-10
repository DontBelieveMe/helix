using System;
using System.Collections.Generic;

namespace Testify
{
    class Program
    {
        private static Dictionary<string, Func<IReportPrinter>> _printers = new Dictionary<string, Func<IReportPrinter>>
        {
            { "json", () => { return new SummaryPrinter(); } },
            { "html", () => { return new SummaryPrinter(); } },
        };

        private static Dictionary<string, Func<ITestsuite>> _testsuites = new Dictionary<string, Func<ITestsuite>>
        {
            { "helix",      () => { return new HelixTestsuite(); } },
            { "ctestsuite", () => { return new CTestsuite(); } },
        };

        private static void RunTestsuite(string name)
        {
            TestRunner runner = new TestRunner(_testsuites[name]());

            Report report = runner.RunAll();

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
