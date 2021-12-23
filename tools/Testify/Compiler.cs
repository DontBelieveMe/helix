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

        private static string GetGCCFilePath()
        {
            return "contrib/bwilks/gcc-arm/bin/arm-none-linux-gnueabihf-gcc.exe";
        }

        public static CompilationResult CompileSingleFile(string tempDir, string sourceFile, string commandLine)
        {
            string sourceFilename = Path.GetFileNameWithoutExtension(sourceFile);

            string tempAssemblyFilePath = tempDir + "/" + sourceFilename + ".S";
            string tempExecutablePath = tempDir + "/" + sourceFilename + ".out";

            string filepath = GetExecutablePath();
            string[] args = { commandLine, sourceFile, "-S", "-o " + tempAssemblyFilePath, "--" };

            Stopwatch stopwatch = Stopwatch.StartNew();
            ProgramOutput output = ProcessHelpers.RunExternalProcess(filepath, string.Join(" ", args));
            stopwatch.Stop();

            string assemblyOuptut = "";
            
            if (output.ExitCode == 0)
                assemblyOuptut = File.ReadAllText(tempAssemblyFilePath);

            CompilationResult result =
                new CompilationResult(output.Stdout, output.Stderr, output.ExitCode, stopwatch.ElapsedMilliseconds, sourceFile, assemblyOuptut, tempExecutablePath);

            result.CompilationCommands.Add(filepath + " " + string.Join(" ", args));

            if (output.ExitCode == 0)
            {
                string gccFilepath = GetGCCFilePath();
                string[] gccArgs = { "-static", "-o " + tempExecutablePath, tempAssemblyFilePath};

                ProgramOutput gccOutput = ProcessHelpers.RunExternalProcess(gccFilepath, string.Join(" ", gccArgs));
                result.CompilationCommands.Add(gccFilepath + " " + string.Join(" ", gccArgs));

                result.AppendStderr(gccOutput.Stderr);
                result.AppendStdout(gccOutput.Stdout);

                result.SetExitCode(gccOutput.ExitCode);
            }

            return result;
        }
    }
}
