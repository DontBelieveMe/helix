using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace Testify
{
    class HelixHelpers
    {
        public static string GetCompilerPath()
        {
            string debugPath = "vs2019/Debug/helix.exe";
            string releasePath = "vs2019/Release/helix.exe";

            if (File.Exists(releasePath))
            {
                return releasePath;
            }
            else
            {
                if (File.Exists(debugPath))
                {
                    return debugPath;
                }

                throw new Exception("Cannot find compiler path");
            }
        }
    }
}
