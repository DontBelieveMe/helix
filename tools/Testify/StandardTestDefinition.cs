using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace Testify
{
    enum TextCompareMode
    {
        Regex,
        PlainText
    }

    class StandardTestDefinition
    {
        public string ExpectedOutput { get; private set; }
        public string CompilationFlags { get; private set; }

        public TestStatus ExpectedStatus { get; private set; }

        public TextCompareMode ExpectedOutputComparisonMode { get; private set; }

        public int ExecutableExpectedExitCode { get; private set; } = 0;

        public static StandardTestDefinition FromXml(string xmlFilepath)
        {
            StandardTestDefinition def = new StandardTestDefinition();

            XmlDocument xmlDocument = new XmlDocument();
            xmlDocument.Load(xmlFilepath);

            XmlNode expectedOutputNode = xmlDocument.GetElementsByTagName("ExpectedOutput")[0];
            XmlNode flagsNode = xmlDocument.GetElementsByTagName("Flags")[0];

            var exeExitCode = xmlDocument.GetElementsByTagName("ExecutableExpectedExitCode");

            if (exeExitCode.Count > 0)
            {
                def.ExecutableExpectedExitCode = Convert.ToInt32(exeExitCode[0].InnerText);
            }

            def.ExpectedOutput = expectedOutputNode.InnerText.Trim().Replace("\r\n", "\n");
            def.CompilationFlags = flagsNode.InnerText;
            def.ExpectedOutputComparisonMode = TextCompareMode.PlainText;
            def.ExpectedStatus = TestStatus.Pass;

            XmlNodeList testFlags = xmlDocument.GetElementsByTagName("TestFlag");

            foreach (XmlNode flag in testFlags)
            {
                string name  = flag.Attributes["name"].Value;
                string value = flag.Attributes["value"].Value;

                if (name == "regex")
                {
                    if (Convert.ToBoolean(value))
                        def.ExpectedOutputComparisonMode = TextCompareMode.Regex;
                    else
                        def.ExpectedOutputComparisonMode = TextCompareMode.PlainText;
                }
                else if (name == "xfail")
                {
                    if (Convert.ToBoolean(value))
                        def.ExpectedStatus = TestStatus.Fail;
                    else
                        def.ExpectedStatus = TestStatus.Pass;
                }
            }

            return def;
        }
    }
}
