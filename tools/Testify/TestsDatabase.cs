using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace Testify
{
    public class Summary
    {
        public int Passes;
        public int Fails;
        public int Total;
        public int Skipped;
        public DateTime When;
    }

    public class TestsDatabase
    {
        private static string _lastRunSummaryJson;

        public static Report ReadLastReport(string testsuiteName)
        {
            FileInfo file = 
                new DirectoryInfo(".testify/" + testsuiteName).GetFiles().OrderByDescending(o => o.CreationTime).FirstOrDefault();

            string json = File.ReadAllText(file.FullName);
            return Report.FromJson(json);
        }

        public static List<Summary> ReadSummaryHistory(string testsuiteName)
        {
            string regressionsDataFilepath = ".testify/" + testsuiteName + "-regressions.json";
            return Newtonsoft.Json.JsonConvert.DeserializeObject<List<Summary>>(File.ReadAllText(regressionsDataFilepath));
        }

        public static void Update(Report report)
        {
            Console.WriteLine();

            if (report.AbortedEarly)
            {
                Console.WriteLine("Not updating JSON test data due to early abort");
                return;
            }

            Console.WriteLine("Updating JSON data...");

            if (!Directory.Exists(".testify/" + report.Name))
            {
                Console.WriteLine("Creating '.testify/" + report.Name + "' directory");
                Directory.CreateDirectory(".testify/" + report.Name);
            }

            List<Summary> summaries = new List<Summary>();

            string regressionsDataFilepath = ".testify/" + report.Name + "-regressions.json";

            if (File.Exists(regressionsDataFilepath))
            {
                Console.WriteLine("Reading test regression history from '" + regressionsDataFilepath + "'");

                summaries = Newtonsoft.Json.JsonConvert.DeserializeObject<List<Summary>>(File.ReadAllText(regressionsDataFilepath));
            }

            summaries.Add(new Summary()
            {
                Passes = report.GetTotalWithStatus(TestStatus.Pass),
                Fails = report.GetTotalWithStatus(TestStatus.Fail),
                Skipped = report.GetTotalWithStatus(TestStatus.Skipped),
                When = report.GenerationTime,
                Total = report.TotalTestsRan
            });

            _lastRunSummaryJson = Newtonsoft.Json.JsonConvert.SerializeObject(summaries);
            

            Console.WriteLine("Writing new test regression history and JSON report");

            File.WriteAllText(".testify/" + report.Name + "-regressions.json", _lastRunSummaryJson);
            File.WriteAllText(".testify/" + report.Name + "/" + report.Name + "-" + report.GenerationTime.ToString("yyyy-MM-dd_HH_mm_ss") + ".json", report.ToJson());
        }

        public static string GetLastRunSummaryJson()
        {
            return _lastRunSummaryJson;
        }
    }
}
