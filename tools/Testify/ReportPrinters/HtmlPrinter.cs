using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using RazorEngine;
using RazorEngine.Templating;

namespace Testify
{

    public class Model
    {
        public Report TestReport;
        public string RegressionJson;

        public string GetStatusColour(TestStatus status)
        {
            switch (status)
            {
                case TestStatus.Pass:
                    return "green";
                case TestStatus.Fail:
                    return "red";
                case TestStatus.XFail:
                    return "orange";
                case TestStatus.Skipped:
                    return "purple";
            }
            return "black";
        }

        public string HtmlId(TestRun testrun)
        {
            return Math.Abs(testrun.Compilation.SourceFile.GetHashCode()).ToString();
        }
    }

    public class HtmlPrinter : IReportPrinter
    {
        private string SourceBase = "tools/Testify/ReportPrinters/Html";
        private string TargetBase = ".testify/html";

        private void CopyDependencies()
        {

            string[] deps = new string[]
            {
                "bootstrap.bundle.min.js",
                "bootstrap.min.css",
                "index.css",
                "index.js",
                "favicon.svg"
            };

            foreach (string dep in deps)
            {
                string inputPath = SourceBase + "/" + dep;
                string outputPath = TargetBase + "/" + dep;

                File.Copy(inputPath, outputPath, true);
            }
        }

        private string GenerateReportJsonDump(Report report)
        {
            return Newtonsoft.Json.JsonConvert.SerializeObject(report);
        }

        private void GenerateHtml(Model model)
        {
            string template = File.ReadAllText(SourceBase + "/" + "index.html");
            string result   = Engine.Razor.RunCompile(template, "html_report", typeof(Model), model);

            File.WriteAllText(TargetBase + "/" + model.TestReport.Name + ".html", result);
        }

        public class Summary
        {
            public int Passes;
            public int Fails;
            public int Total;
            public int Skipped;
            public DateTime When;
        }

        public void Print(Report report)
        {
            if (report.AbortedEarly)
                return;

            if (!Directory.Exists(".testify/html"))
            {
                Directory.CreateDirectory(".testify/html");
            }

            if (!Directory.Exists(".testify/" + report.Name))
            {
                Directory.CreateDirectory(".testify/" + report.Name);
            }

            List<Summary> summaries = new List<Summary>();

            if (File.Exists(".testify/" + report.Name + "-regressions.json"))
            {
                summaries = Newtonsoft.Json.JsonConvert.DeserializeObject<List<Summary>>(File.ReadAllText(".testify/" + report.Name + "-regressions.json"));
            }

            summaries.Add(new Summary()
            {
                Passes = report.GetTotalWithStatus(TestStatus.Pass),
                Fails = report.GetTotalWithStatus(TestStatus.Fail),
                Skipped = report.GetTotalWithStatus(TestStatus.Skipped),
                When = report.GenerationTime,
                Total = report.TotalTestsRan
            });

            string jsonSummary = Newtonsoft.Json.JsonConvert.SerializeObject(summaries);

            File.WriteAllText(".testify/" + report.Name + "-regressions.json", jsonSummary);
            File.WriteAllText(".testify/" + report.Name + "/" + report.Name + "-" + report.GenerationTime.ToString("yyyy-MM-dd_HH_mm_ss") + ".json", report.ToJson());

            CopyDependencies();
            GenerateHtml(new Model() { TestReport = report, RegressionJson = jsonSummary });
        }
    }
}
