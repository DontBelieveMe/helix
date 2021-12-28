using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace Testify
{
    public class TestsDatabase
    {
        public class Summary
        {
            public int Passes;
            public int Fails;
            public int Total;
            public int Skipped;
            public DateTime When;
        }

        private static string _lastRunSummaryJson;

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
