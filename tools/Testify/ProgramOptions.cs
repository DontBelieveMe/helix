using System;
using System.Collections.Generic;
using System.Text;

namespace Testify
{
    enum SummaryMode
    {
        Short,
        Long
    }

    class ProgramOptions
    {
        public static string[] ReportFormats { get; private set; } = new string[] { };
        public static string[] Testsuites { get; private set; } = new string[] { };

        public static bool FailFast { get; private set; } = false;
        public static bool DumpDiffs { get; private set; } = false;

        public static bool Verbose { get; private set; } = false;

        public static bool SummariseLastRun { get; private set; } = false;
        public static SummaryMode SummaryMode { get; private set; } = SummaryMode.Short;

        public static bool ReportOnly { get; private set; } = false;

        public static string[] PositionalArguments { get; private set; } = new string[] { };

        private static string HelpText =
@"Usage: Testify [<Options>] Positional...

Optional:
  -report=?                    Set the output report type (summary report is implicit & default)
  -testsuite=?                 Set the testsuite (see 'Testsuites')

  -fail-fast                   Abort the test run after the first failure. Runs that terminate early don't
                               get recorded.
  -dump-diffs                  For any tests that fail print out the expected & actual output
  -verbose                     For each run test print out the command line used to run that test
  -summarise-last              Print out a summary of the previous test run, don't run any new tests.
  -summary-mode=[long|short]   Used in conjunction with '-summarise-last'. 'long' summary mode means to print
                               out each run test & it's individual status, 'short' is to just print out a summary.
                               Default is 'short'.
  -report-only                 Don't run any tests & just generate a new report based of previous test run.
  -help                        Print out this help text & exit - but i guess you figured that out already :-)

Testsuites:
  helix                        Helix compilers own regression & integration tests.

                               An optional positional argument is available, which specifies the subfolder
                               (in 'testsuite/') to run. If a subfolder is specified test runs are not recorded.

  ctestsuite                   Open source c-testsuite (https://github.com/c-testsuite/c-testsuite)
";

        public static void Parse(string[] args)
        {
            List<string> testsuites = new List<string>();
            List<string> reports = new List<string>();
            List<string> positionals = new List<string>();

            foreach (string arg in args)
            {
                if (arg.StartsWith("-report="))
                {
                    reports.Add(arg.Split("=")[1]);
                }
                else if (arg.StartsWith("-testsuite="))
                {
                    testsuites.Add(arg.Split("=")[1]);
                }
                else if (arg == "-dump-diffs")
                {
                    DumpDiffs = true;
                }
                else if (arg == "-fail-fast")
                {
                    FailFast = true;
                }
                else if (arg == "-verbose")
                {
                    Verbose = true;
                }
                else if (arg == "-summarise-last")
                {
                    SummariseLastRun = true;
                }
                else if (arg == "-report-only")
                {
                    ReportOnly = true;
                }
                else if (arg == "-help" || arg == "--help")
                {
                    Console.WriteLine(HelpText);
                    Environment.Exit(0);
                }
                else if (arg.StartsWith("-summary-mode="))
                {
                    string mode = arg.Split("=")[1];

                    if (mode == "short")
                        SummaryMode = SummaryMode.Short;
                    else if (mode == "long")
                        SummaryMode = SummaryMode.Long;
                }
                else if (!arg.StartsWith("-"))
                {
                    positionals.Add(arg);
                }
            }

            ReportFormats = reports.ToArray();
            Testsuites = testsuites.ToArray();
            PositionalArguments = positionals.ToArray();
        }
    }
}
