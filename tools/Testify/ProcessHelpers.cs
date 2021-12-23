using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Text;

namespace Testify
{
    public class ProgramOutput
    {
        public string Stdout;
        public string Stderr;
        public int ExitCode;
        public long Time;
    }

    class ProcessHelpers
    {
        public static ProgramOutput RunExternalProcess(string name, string args)
        {
            StringBuilder stdout = new StringBuilder();
            StringBuilder stderr = new StringBuilder();
            int exitCode;
            long time;

            using (Process process = new Process())
            {
                process.StartInfo.FileName = name;
                process.StartInfo.Arguments = args;
                process.StartInfo.CreateNoWindow = true;
                process.StartInfo.UseShellExecute = false;
                process.StartInfo.RedirectStandardOutput = true;
                process.StartInfo.RedirectStandardError = true;
                process.OutputDataReceived += (sender, args) => stdout.AppendLine(args.Data);
                process.ErrorDataReceived += (sender, args) => stderr.AppendLine(args.Data);

                process.Start();
                process.BeginOutputReadLine();
                if (process.WaitForExit(2000))
                    exitCode = process.ExitCode;
                else
                    exitCode = 1;

                time = (long)process.TotalProcessorTime.TotalMilliseconds;
            }

            return new ProgramOutput { Stderr = stderr.ToString(), Stdout = stdout.ToString(), ExitCode = exitCode, Time = time };
        }
    }
}
