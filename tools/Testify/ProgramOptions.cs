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

        public static void Parse(string[] args)
        {
            List<string> testsuites = new List<string>();
            List<string> reports = new List<string>();

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
                else if (arg.StartsWith("-summary-mode="))
                {
                    string mode = arg.Split("=")[1];

                    if (mode == "short")
                        SummaryMode = SummaryMode.Short;
                    else if (mode == "long")
                        SummaryMode = SummaryMode.Long;
                }
            }

            ReportFormats = reports.ToArray();
            Testsuites = testsuites.ToArray();
        }
    }
}
