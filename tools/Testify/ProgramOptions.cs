using System;
using System.Collections.Generic;
using System.Text;

namespace Testify
{
    class ProgramOptions
    {
        public static string[] ReportFormats { get; private set; } = new string[] { };
        public static string[] Testsuites { get; private set; } = new string[] { };

        public static bool FailFast { get; private set; } = false;
        public static bool DumpDiffs { get; private set; } = false;

        public static bool Verbose { get; private set; } = false;

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
            }

            ReportFormats = reports.ToArray();
            Testsuites = testsuites.ToArray();
        }
    }
}
