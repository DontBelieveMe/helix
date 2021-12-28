using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;

namespace Testify
{
    class HelixCompiler
    {
        private static string GetCompilerExecutablePath()
        {
            // List of executable paths to try to find the compiler
            // Priority is top down (e.g. item at the top is most desirable)
            string[] executablePaths =
            {
                "vs2022/src/hxc/Release/hxc.exe",
                "vs2022/src/hxc/Debug/hxc.exe",

                "vs2019/src/hxc/Release/hxc.exe",
                "vs2019/src/hxc/Debug/hxc.exe"
            };

            foreach (string executablePath in executablePaths)
            {
                if (File.Exists(executablePath))
                {
                    return executablePath;
                }
            }

            throw new Exception("Cannot suitable helix compiler executable");
        }

        public static CompilationResult CompileSingleFile(string sourceFile, string[] extraCompilationFlags, string outputExecutableFilepath)
        {
            // Path to the compiler executable
            string compilerFilepath = GetCompilerExecutablePath();

            // Want to put any temporary files in the same folder as the executable.
            string temporaryFilesDirectory = Path.GetDirectoryName(outputExecutableFilepath);

            // List of arguments to pass to the compiler
            string[] compilerArguments =
            {
                sourceFile,                              // The file we want to compile
                
                "-o " + outputExecutableFilepath,        // Set the correct output file

                "--no-colours",                          // Want any output as plain text, with no terminal/ascii colouring
                "--save-temps",                          // Save temporary files

                "--dumpbase " + temporaryFilesDirectory, // Set the base directory for temporary files (e.g. assembly files)
                                                         // Want to explictly set this incase the compiler ICEs and doesn't clean
                                                         // up temporary files (don't want them in an unknown location)

                string.Join(" ", extraCompilationFlags)  // Any extra arguments passed down
            };

            // Bingo! Finally invoke the compiler process :-)
            ProgramOutput output = ProcessHelpers.RunExternalProcess(compilerFilepath, compilerArguments);

            return new CompilationResult(
                output.Stdout,
                output.Stderr,
                output.ExitCode,
                sourceFile,
                outputExecutableFilepath,
                output.Time,
                string.Format("{0} {1}", compilerFilepath, string.Join(" ", compilerArguments))
            );
        }
    }
}
