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

        public string GetSourceCode(TestRun run)
        {
            return File.ReadAllText(run.Compilation.SourceFile);
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

        public void Print(Report report)
        {
            if (report.AbortedEarly)
                return;

            if (!Directory.Exists(".testify/html"))
            {
                Directory.CreateDirectory(".testify/html");
            }

            CopyDependencies();
            GenerateHtml(new Model() { TestReport = report, RegressionJson = TestsDatabase.GetLastRunSummaryJson() });
        }
    }
}
