using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MachineDescription
{
    class ProgramOptions
    {
        public static bool PrintTokens { get; private set; } = false;
        public static bool PrintAST { get; private set; } = false;
        public static string OutputDirectory { get; private set; } = "";
        public static string SourceFile { get; private set; }

        public static bool Parse(string[] options)
        {
            foreach (string arg in options)
            {
                if (arg == "-print-tokens")
                    PrintTokens = true;
                else if (arg == "-print-ast")
                    PrintAST = true;
                else if (arg.StartsWith("-output="))
                    OutputDirectory = arg.Split("=")[1];
                else
                {
                    if (SourceFile != null)
                    {
                        Console.Error.WriteLine(" > Error: Cannot specify multiple source files (didn't expect '{}')", arg);
                        return false;
                    }

                    SourceFile = arg;
                }
            }
            
            return true;
        }
    }
}
