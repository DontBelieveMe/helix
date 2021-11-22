using System;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;

namespace Testify
{
    class ProgramOutput
    {
        public string Stdout;
        public string Stderr;
    }

    class TestsuiteStats
    {
        public int Passes = 0;
        public int Fails = 0;

        public int TotalRuns {  get { return Passes + Fails; } }

        public double CalculatePassPercentage()
        {
            return (Passes / (double)TotalRuns) * 100;
        }

        public double CalculateFailPercentage()
        {
            return (Fails / (double)TotalRuns) * 100;
        }
    }

    class Program
    {
        static ProgramOutput RunExternalProcess(string name, string args)
        {
            StringBuilder stdout = new StringBuilder();
            StringBuilder stderr = new StringBuilder();

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
            }

            return new ProgramOutput { Stderr = stderr.ToString(), Stdout = stdout.ToString() };
        }

        static void RunTestFromXmlDefinition(string testfile, TestsuiteStats stats)
        {
            XmlDocument xmlDocument = new XmlDocument();
            xmlDocument.Load(testfile);

            XmlNode expectedOutputNode = xmlDocument.GetElementsByTagName("ExpectedOutput")[0];
            XmlNode flagsNode = xmlDocument.GetElementsByTagName("Flags")[0];

            string expectedOutputString = expectedOutputNode.InnerText.Trim();
            string flagsString = flagsNode.InnerText;
            string sourceFile = testfile.Replace(".xml", ".c");
            flagsString = sourceFile + " " + flagsString;
            flagsString += " --";

            Console.Write("{0}... ", sourceFile);

            ProgramOutput output = RunExternalProcess("vs2019/Debug/helix.exe", flagsString);

            bool matchesExpectedOuptut = Regex.IsMatch(output.Stdout, expectedOutputString, RegexOptions.Multiline);

            if (matchesExpectedOuptut)
            {
                Console.ForegroundColor = ConsoleColor.Green;
                Console.WriteLine("[pass]");
                Console.ResetColor();
                stats.Passes++;
            }
            else
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[fail]");
                Console.ResetColor();

                Console.WriteLine("------------------------- Expected --------------------------");
                Console.WriteLine(expectedOutputString);
                Console.WriteLine("------------------------- Actual ----------------------------");
                Console.WriteLine(output.Stdout);
                stats.Fails++;
            }

        }

        static void Main(string[] args)
        {
            TestsuiteStats stats = new TestsuiteStats();
            
            foreach (string file in Directory.GetFiles("testsuite\\f0", "*.xml", SearchOption.AllDirectories))
            {
                RunTestFromXmlDefinition(file, stats);
            }

            Console.WriteLine();
            Console.WriteLine("[Summary]");

            Console.Write("\t");
            Console.ForegroundColor = ConsoleColor.Green;
            Console.Write("PASSES");
            Console.ResetColor();
            Console.WriteLine(": {0} ({1}%)", stats.Passes, stats.CalculatePassPercentage());

            Console.Write("\t");
            Console.ForegroundColor = ConsoleColor.Red;
            Console.Write("FAILS");
            Console.ResetColor();
            Console.WriteLine(":  {0} ({1}%)", stats.Fails, stats.CalculateFailPercentage());

            Console.WriteLine("\tTotal:  {0}", stats.TotalRuns);
        }
    }
}
