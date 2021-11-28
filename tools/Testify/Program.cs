using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;

namespace Testify
{
    class ProgramOutput
    {
        public string Stdout;
        public string Stderr;
        public int ExitCode;
    }

    class TestsuiteStats
    {
        public int Passes = 0;
        public int Fails = 0;
        public int XFails = 0;

        public int TotalRuns {  get { return Passes + Fails + XFails; } }

        public double CalculatePassPercentage()
        {
            return (Passes / (double)TotalRuns) * 100;
        }

        public double CalculateFailPercentage()
        {
            return (Fails / (double)TotalRuns) * 100;
        }

        public double CalculateXFailPercentage()
        {
            return (XFails / (double)TotalRuns) * 100;
        }

        public List<long> ExecutionTimes = new List<long>();

        public long GetSmallestExecutionTime()
        {
            return ExecutionTimes.Min();
        }

        public long GetLargestExecutionTime()
        {
            return ExecutionTimes.Max();
        }

        public double GetAverageExecutionTime()
        {
            return ExecutionTimes.Average();
        }
    }

    class Options
    {
        public bool DumpDiffs = false;
        public List<string> SpecificTests = new List<string>();

        public static Options Parse(string[] args)
        {
            Options opts = new Options();
            foreach (string arg in args)
            {
                if (arg == "-dump-diffs")
                    opts.DumpDiffs = true;
                else
                {
                    opts.SpecificTests.Add(arg.Replace(".c", ".xml"));
                }
            }
            return opts;
        }
    }

    class Program
    {
        static ProgramOutput RunExternalProcess(string name, string args)
        {
            StringBuilder stdout = new StringBuilder();
            StringBuilder stderr = new StringBuilder();
            int exitCode;

            using (Process process = new Process())
            {
                process.StartInfo.FileName = name;
                process.StartInfo.Arguments = args;
                process.StartInfo.CreateNoWindow = true;
                process.StartInfo.UseShellExecute = false;
                process.StartInfo.RedirectStandardOutput = true;
                process.StartInfo.RedirectStandardError = true;
                process.OutputDataReceived += (sender, args) => stdout.AppendLine(args.Data);
                process.ErrorDataReceived += (sender, args) => stderr.AppendLine(args.Data);

                process.Start();
                process.BeginOutputReadLine();
                process.WaitForExit();
                exitCode = process.ExitCode;
            }

            return new ProgramOutput { Stderr = stderr.ToString(), Stdout = stdout.ToString(),  ExitCode = exitCode };
        }

        static void RunTestFromXmlDefinition(string testfile, TestsuiteStats stats, Options opts)
        {
            XmlDocument xmlDocument = new XmlDocument();
            xmlDocument.Load(testfile);

            XmlNode expectedOutputNode = xmlDocument.GetElementsByTagName("ExpectedOutput")[0];
            XmlNode flagsNode = xmlDocument.GetElementsByTagName("Flags")[0];

            string expectedOutputString = expectedOutputNode.InnerText.Trim().Replace("\r\n", "\n");
            string flagsString = flagsNode.InnerText;
            string sourceFile = testfile.Replace(".xml", ".c");
            flagsString = sourceFile + " " + flagsString;
            flagsString += " --";

            bool useRegex = false;
            bool xfail = false;

            XmlNodeList testFlags = xmlDocument.GetElementsByTagName("TestFlag");

            foreach (XmlNode flag in testFlags)
            {
                string name = flag.Attributes["name"].Value;
                string value = flag.Attributes["value"].Value;

                if (name == "regex")
                {
                    useRegex = Convert.ToBoolean(value);
                }
                else if (name == "xfail")
                {
                    xfail = Convert.ToBoolean(value);
                }
            }

            Console.Write("{0}... ", sourceFile);

            Stopwatch timer = Stopwatch.StartNew();
            ProgramOutput output = RunExternalProcess("vs2019/Debug/helix.exe", flagsString);
            timer.Stop();

            stats.ExecutionTimes.Add(timer.ElapsedMilliseconds);

            string stdout = output.Stdout.Trim().Replace("\r\n", "\n");

            bool matchesExpectedOuptut = false;
            
            if (useRegex)
            {
                matchesExpectedOuptut = Regex.IsMatch(stdout, expectedOutputString, RegexOptions.Multiline);
            }
            else
            {
                matchesExpectedOuptut = (stdout == expectedOutputString);
            }

            bool expectedExitCode = output.ExitCode == 0;

            if (matchesExpectedOuptut && expectedExitCode)
            {
                Console.ForegroundColor = ConsoleColor.Green;
                Console.Write("[pass]");

                if (xfail)
                {
                    Console.ForegroundColor = ConsoleColor.DarkYellow;
                    Console.Write(" [marked as xfail??]");
                }

                Console.WriteLine();

                Console.ResetColor();

                stats.Passes++;
            }
            else
            {
                if (xfail)
                {
                    Console.ForegroundColor = ConsoleColor.DarkYellow;
                    Console.WriteLine("[xfail]");
                    Console.ResetColor();

                    stats.XFails++;
                }
                else
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine("[fail]");
                    Console.ResetColor();

                    if (!matchesExpectedOuptut && opts.DumpDiffs)
                    {
                        Console.WriteLine("------------------------- Expected --------------------------");
                        Console.WriteLine(expectedOutputString);
                        Console.WriteLine("------------------------- Actual ----------------------------");
                        Console.WriteLine(stdout);
                        Console.WriteLine("-------------------------------------------------------------");
                    }

                    stats.Fails++;
                }
            }

        }

        static void Main(string[] args)
        {
            Options opts = Options.Parse(args);

            TestsuiteStats stats = new TestsuiteStats();

            Stopwatch timer = Stopwatch.StartNew();

            if (opts.SpecificTests.Count > 0)
            {
                foreach (string file in opts.SpecificTests)
                {
                    RunTestFromXmlDefinition(file, stats, opts);
                }
            }
            else
            {
                foreach (string file in Directory.GetFiles("testsuite", "*.xml", SearchOption.AllDirectories))
                {
                    RunTestFromXmlDefinition(file, stats, opts);
                }
            }

            timer.Stop();

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
                stats.TotalRuns, timer.ElapsedMilliseconds / 1000.0, stats.GetSmallestExecutionTime(), stats.GetLargestExecutionTime(),stats.GetAverageExecutionTime());
            Console.WriteLine();

//            Console.WriteLine("\tTotal:  {0}", stats.TotalRuns);
  //          Console.WriteLine();
            //Console.WriteLine("\tTotal Runtime: {0}s", timer.ElapsedMilliseconds / 1000.0);
            //Console.WriteLine("\t   [Smallest]: {0}ms", stats.GetSmallestExecutionTime());
            //Console.WriteLine("\t   [Largest]:  {0}ms", stats.GetLargestExecutionTime());
            //Console.WriteLine("\t   [Average]:  {0}ms", stats.GetAverageExecutionTime());
        }
    }
}
