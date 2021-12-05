using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;

using Newtonsoft.Json;

namespace Testify
{
    class CTestsuite
    {
        class Summary
        {
            public int Passes;
            public int Fails;
            public int Total;
            public int Skipped;
            public DateTime When;
        }

        class TestRun
        {
            public string Status;
            public int ExecutionTime;
            public string TestFile;
            public string Tags = "";
        }

        public void Run()
        {
            Console.WriteLine("Running c-testsuite tests (https://github.com/c-testsuite/c-testsuite)...");

            if (!Directory.Exists("extras/c-testsuite"))
            {
                Console.Write("[");
                Console.ForegroundColor = ConsoleColor.Red;
                Console.Write("error");
                Console.ResetColor();
                Console.Write("] extras/c-testsuite does not exist?");
                return;
            } else
            {
                Console.WriteLine("Found extras/c-testsuite directory...");
            }

            TestsuiteStats stats = new TestsuiteStats();
            Stopwatch timer = Stopwatch.StartNew();

            List<TestRun> runs = new List<TestRun>();

            foreach (string file in Directory.GetFiles("extras/c-testsuite/tests/single-exec/", "*.c", SearchOption.AllDirectories))
            {
                string tagsFilepath = file + ".tags";

                string[] tags = File.ReadAllLines(tagsFilepath);

                bool skip = false;

                if (tags.Contains("needs-libc"))
                    skip = true;

                string[] args = { file, "--" };

                if (skip)
                {
                    TestRun skippedRun = new TestRun();
                    skippedRun.ExecutionTime = 0;
                    skippedRun.TestFile = file;
                    skippedRun.Tags = string.Join("; ", tags);
                    skippedRun.Status = "skipped";

                    stats.ExecutionTimes.Add(0);
                    stats.Skipped += 1;

                    runs.Add(skippedRun);

                    continue;
                }

                Stopwatch stopwatch = Stopwatch.StartNew();
                ProgramOutput output = ProcessHelpers.RunExternalProcess("vs2019/Debug/helix.exe", string.Join(" ", args));
                stopwatch.Stop();
                stats.ExecutionTimes.Add(stopwatch.ElapsedMilliseconds);

                Console.Write(file + ".. .");

                TestRun run = new TestRun();
                run.ExecutionTime = (int) stopwatch.ElapsedMilliseconds;
                run.TestFile = file;
                run.Tags = string.Join("; ", tags);

                if (output.ExitCode == 0)
                {
                    Console.Write("[");
                    Console.ForegroundColor = ConsoleColor.Green;
                    Console.Write("pass");
                    Console.ResetColor();
                    Console.WriteLine("]");
                    stats.Passes += 1;
                    run.Status = "pass";
                }
                else
                {
                    Console.Write("[");
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.Write("fail");
                    Console.ResetColor();
                    Console.WriteLine("]");
                    stats.Fails += 1;
                    run.Status = "fail";
                }

                runs.Add(run);
            }

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
            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.Write("SKIPPED");
            Console.ResetColor();
            Console.WriteLine(":  {0} ({1:0.00}%)", stats.Skipped, stats.CalculateSkippedPercentage());

            Console.Write("\t");
            Console.ForegroundColor = ConsoleColor.DarkYellow;
            Console.Write("XFAILS");
            Console.ResetColor();
            Console.WriteLine(": {0} ({1:0.00}%)", stats.XFails, stats.CalculateXFailPercentage());

            Console.WriteLine();
            Console.WriteLine("Executing {0} tests took {1:0.00}s (average of {4:0.00}ms each, range {2}ms -> {3}ms)",
                stats.TotalRuns, timer.ElapsedMilliseconds / 1000.0, stats.GetSmallestExecutionTime(), stats.GetLargestExecutionTime(), stats.GetAverageExecutionTime());
            Console.WriteLine();

            string json = JsonConvert.SerializeObject(runs);
            json = "let results=" + json;

            DateTime creationTime = File.GetLastWriteTime("tools/testify-web/data/c-testsuite.json");

            File.Copy("tools/testify-web/data/c-testsuite.json", "tools/testify-web/data/c-testsuite-" + creationTime.ToString("yyyy-MM-dd_HH_mm_ss") + ".json");

            File.WriteAllText("tools/testify-web/data/c-testsuite.json", json);

            List<Summary> summaries;

            if (File.Exists("tools/testify-web/data/c-testsuite-summary.json"))
            {

                string summaryText = File.ReadAllText("tools/testify-web/data/c-testsuite-summary.json");
                summaryText = summaryText.Replace("let testsummaries =", "");
                summaries = JsonConvert.DeserializeObject<List<Summary>>(summaryText);
            }
            else
            {
                summaries = new List<Summary>();
            }

            summaries.Add(new Summary { Passes = stats.Passes, Fails = stats.Fails, Total = stats.TotalRuns, When = DateTime.Now, Skipped = stats.Skipped });

            string summaryJsonText = JsonConvert.SerializeObject(summaries);
            summaryJsonText = "let testsummaries =" + summaryJsonText;

            File.WriteAllText("tools/testify-web/data/c-testsuite-summary.json", summaryJsonText);
        }
    }
}
