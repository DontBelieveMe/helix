using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;

namespace Testify
{
    class HelixCompiler
    {
        private static string GetExecutablePath()
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

        public static CompilationResult CompileSingleFile(string sourceFile, string commandLine)
        {
            string filepath = GetExecutablePath();
            string[] args = { commandLine, sourceFile, "--" };

            Stopwatch stopwatch = Stopwatch.StartNew();
            ProgramOutput output = ProcessHelpers.RunExternalProcess(filepath, string.Join(" ", args));
            stopwatch.Stop();
            return new CompilationResult(output.Stdout, output.Stderr, output.ExitCode, stopwatch.ElapsedMilliseconds, sourceFile);
        }
    }
}
