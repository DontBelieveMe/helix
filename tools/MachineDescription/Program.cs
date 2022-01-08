using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace MachineDescription
{
    class Instruction
    {
        public string Name;
        public string OutputFormat;
        public IRInstructionTemplate Template;
    }

    class IROperandTemplate
    {
        public string Type;
        public virtual string GenerateCondition(string value) { return "false"; }
    }

    class IRInstructionTemplate
    {
        public string Opcode;
        public List<IROperandTemplate> Operands = new List<IROperandTemplate>();
        private Instruction _parent;

        public IRInstructionTemplate(SExpression array, Instruction parent)
        {
            _parent = parent;
            Opcode = (array.Children[0] as Symbol).Value;

            for (int i = 1; i < array.Children.Count; ++i)
            {
                SExpression operand = array.GetChildAs<SExpression>(i);
                string[] fullOperandClass = operand.GetChildAs<Symbol>(0).Value.Split(":");
                string operandClass = fullOperandClass[0];
                string operandType = fullOperandClass[1];

                if (operandClass == "match_operand")
                {
                    Operands.Add(new MatchOperand(operandType, operand));
                }
                else if (operandClass == "const_int")
                {
                    Operands.Add(new ConstIntOperand(operandType, operand));
                }
            }
        }

        private Dictionary<string, string> typeMap = new Dictionary<string, string>()
        {
            { "i8", "BuiltinTypes::GetInt8" },
            { "i16", "BuiltinTypes::GetInt16" },
            { "i32", "BuiltinTypes::GetInt32" },
            { "ptr", "BuiltinTypes::GetPointer" },
            { "lbl", "BuiltinTypes::GetLabelType" }
        };

        public void GenerateCodeToMatchTemplate(PrintingContext ctx)
        {
            ctx.PrintIndentedLine(string.Format("if (insn.GetOpcode() == {0}) {{", Opcode));
            ctx.IncreaseIndent(1);

            if (Operands.Count > 0)
            {
                ctx.PrintIndentedLine("if (");
                ctx.IncreaseIndent(1);
                
                int index = 0;
                foreach (IROperandTemplate operand in Operands)
                {
                    string line = "   ";

                    if (index > 0)
                        line = "&& ";

                    line += "insn.GetOperand(" + index + ")->GetType() == " + typeMap[operand.Type] + "() && ";
                    line += operand.GenerateCondition("insn.GetOperand(" + index + ")");

                    ctx.PrintIndentedLine(line);
                    index++;
                }

                ctx.DecreaseIndent(1);
                ctx.PrintIndentedLine(") {");
            }
            else
            {
                ctx.PrintIndentedLine("{");
            }

            ctx.IncreaseIndent(1);
            
            // got a match
            {
                string outputString = _parent.OutputFormat;

                List<string> outputLines = new List<string>();

                {
                    string[] lines = outputString.Split("\n");

                    foreach (string line in lines)
                    {
                        outputLines.Add(line.Trim());
                    }
                }

                if (!outputString.Contains("{") || outputString.StartsWith("@"))
                {
                    foreach (string asmLine in outputLines)
                    {
                        string tmp = asmLine;
                        if (tmp.StartsWith("@"))
                            tmp = tmp.Substring(1);

                        ctx.PrintIndentedLine("fprintf(file, \"\\t" + tmp + "\\n\");");
                    }
                }
                else
                {
                    List<string> formatArgs = new List<string>();

                    for (int i = 0; i < Operands.Count; ++i)
                    {
                        if (Operands[i] is MatchOperand)
                        {
                            MatchOperand mo = (MatchOperand)Operands[i];

                            ctx.PrintIndentedLine(string.Format("const std::string& a{0} = stringify_operand(insn.GetOperand({1}), slots);", mo.OutputOperandIndex, i));
                            formatArgs.Add("a" + mo.OutputOperandIndex);
                        }
                    }

                    string args = string.Join(", ", formatArgs);

                    foreach (string asmLine in outputLines)
                    {
                        ctx.PrintIndentedLine(string.Format("{{const std::string as = fmt::format(\"\\t{0}\\n\", {1});", asmLine, args));
                        ctx.PrintIndentedLine("fprintf(file, \"%s\",as.c_str());}");
                    }
                }

                ctx.PrintIndentedLine("return;");
            }

            ctx.DecreaseIndent(1);
            ctx.PrintIndentedLine("}");
            ctx.DecreaseIndent(1);
            ctx.PrintIndentedLine("}");

        }
    }

    class MatchOperand : IROperandTemplate
    {
        public int OutputOperandIndex;
        public string OperandClass;

        public MatchOperand(string type, SExpression sexpr)
        {
            base.Type = type;

            this.OperandClass = sexpr.GetChildAs<String>(2).Value;
            this.OutputOperandIndex = sexpr.GetChildAs<Number>(1).Value;
        }

        public override string GenerateCondition(string value)
        {
            return "is_" + this.OperandClass + "(" + value + ")";
        }
    }

    class ConstIntOperand : IROperandTemplate
    {
        public int ConstantIntegralValue;

        public ConstIntOperand(string type, SExpression sexpr)
        {
            base.Type = type;
            this.ConstantIntegralValue = sexpr.GetChildAs<Number>(1).Value;
        }

        public override string GenerateCondition(string value)
        {
            return "is_const_int_with_value(" + value + ", " + ConstantIntegralValue.ToString() + ")";
        }
    }

    class MachineDescription
    {
        public List<Instruction> Instructions = new List<Instruction>();

        public MachineDescription(List<Node> nodes)
        {
            foreach (Node node in nodes)
            {
                SExpression sexpr = node as SExpression;

                if (sexpr != null)
                {
                    if (sexpr.Children[0].HasValue("define-insn"))
                    {
                        Instructions.Add(ParseInstruction(sexpr));
                    }
                }
            }
        }

        private Instruction ParseInstruction(SExpression node)
        {
            if (node.Children.Count > 4)
            {
                Console.WriteLine(" > Error, too many parameters passed to define-insn");
                Environment.Exit(1);
            }

            String name = node.GetChildAs<String>(1);
            SExpression template = node.GetChildAs<Array>(2).GetChildAs<SExpression>(0);

            String outputFormat = node.GetChildAs<String>(3);

            Instruction insn = new Instruction() { 
                Name = name.Value,
                OutputFormat = outputFormat.Value
            };
            insn.Template = new IRInstructionTemplate(template, insn);

            return insn;
        }
    }

    class MachineDescriptionCodeGenerator
    {
        private List<Node> _nodes;
        private string _machineName;

        private MachineDescription _desc;

        private string Capitalise(string s)
        {
            if (string.IsNullOrWhiteSpace(s))
                return s;

            if (!char.IsUpper(s[0]))
            {
                return char.ToUpper(s[0]) + s.Substring(1);
            }

            return s;
        }

        public MachineDescriptionCodeGenerator(List<Node> nodes, string machineName)
        {
            _nodes = nodes;
            _machineName = machineName;
            _desc = new MachineDescription(nodes);
        }

        private void GenerateHeader(StringBuilder headerFile)
        {
            Console.WriteLine(" > Generating header file contents");

            PrintingContext ctx = new PrintingContext(headerFile);

            // Disclaimer/file header
            {
                headerFile.AppendLine("/**");
                headerFile.AppendLine(" * This file is generated from arm.md by the MachineDescription tool");
                headerFile.AppendLine(" * Do not edit! Any changes will be overwritten by the next invocation");
                headerFile.AppendLine(" * of the tool.");
                headerFile.AppendLine(" */");
            }

            headerFile.AppendLine("#pragma once");
            headerFile.AppendLine("");

            // Includes
            {
                headerFile.AppendLine("#include <stdio.h>");
                headerFile.AppendLine("#include \"opcodes.h\"");
            }

            // Forward declarations
            {
                headerFile.AppendLine("");
                headerFile.AppendLine("namespace Helix");
                headerFile.AppendLine("{");
                headerFile.AppendLine("\tclass Instruction;");
                headerFile.AppendLine("\tclass SlotTracker;");
                headerFile.AppendLine("}");
                headerFile.AppendLine("");
            }

            // Namespace
            {
                headerFile.AppendLine("namespace Helix::ARMv7");
                headerFile.AppendLine("{");

                ctx.IncreaseIndent(1);

                // Opcode enum
                {
                    ctx.PrintIndentedLine("enum Opcode : OpcodeType");
                    ctx.PrintIndentedLine("{");
                    ctx.IncreaseIndent(1);

                    foreach (Instruction insn in _desc.Instructions)
                    {
                        ctx.PrintIndentedLine(Capitalise(insn.Name) + ",");
                    }

                    ctx.DecreaseIndent(1);
                    ctx.PrintIndentedLine("};");
                }

                headerFile.AppendLine("");

                // Function prototypes
                {
                    ctx.PrintIndentedLine("void Emit(FILE*, Instruction&, SlotTracker&);");
                }

                ctx.DecreaseIndent(1);
                headerFile.AppendLine("}");
            }
        }

        private void GenerateSource(StringBuilder sourceFile)
        {
            Console.WriteLine(" > Generating source file contents");

            PrintingContext ctx = new PrintingContext(sourceFile);

            // Disclaimer/file header
            {
                sourceFile.AppendLine("/**");
                sourceFile.AppendLine(" * This file is generated from arm.md by the MachineDescription tool");
                sourceFile.AppendLine(" * Do not edit! Any changes will be overwritten by the next invocation");
                sourceFile.AppendLine(" * of the tool.");
                sourceFile.AppendLine(" */");
            }

            // Includes
            {
                ctx.PrintIndentedLine("");
                ctx.PrintIndentedLine("#include \"arm-md.h\"");
                ctx.PrintIndentedLine("#include \"../src/instructions.h\"");
                ctx.PrintIndentedLine("#include \"../src/system.h\"");
                ctx.PrintIndentedLine("#include \"../src/print.h\"");
                ctx.PrintIndentedLine("");
            }

            // Emit(FILE*,Instruction&)
            {
                ctx.PrintIndentedLine("void Helix::ARMv7::Emit(FILE* file, Instruction& insn, SlotTracker& slots)\n{");
                ctx.IncreaseIndent(1);

                foreach(Instruction insn in _desc.Instructions)
                {
                    insn.Template.GenerateCodeToMatchTemplate(ctx);
                }

                ctx.PrintIndentedLine("helix_unreachable(\"cannot match instruction to assembly, check arm.md\");");
                ctx.DecreaseIndent(1);
                ctx.PrintIndentedLine("}");
            }
        }

        public void Generate(string outputDirectory)
        {
            if (!Directory.Exists(outputDirectory))
            {
                Console.WriteLine(" > Creating directory '{0}', since it does not exist", outputDirectory);
                Directory.CreateDirectory(outputDirectory);
            }

            StringBuilder header = new StringBuilder();
            StringBuilder source = new StringBuilder();

            GenerateHeader(header);
            GenerateSource(source);

            string outputHeaderFilepath = Path.Combine(outputDirectory, _machineName + "-md.h");
            string outputSourceFilepath = Path.Combine(outputDirectory, _machineName + "-md.cpp");

            Console.WriteLine(" > Writing header to '{0}'", outputHeaderFilepath);
            Console.WriteLine(" > Writing source to '{0}'", outputSourceFilepath);

            //Console.WriteLine(header.ToString());
            //Console.WriteLine(source.ToString());

            File.WriteAllText(outputHeaderFilepath, header.ToString());
            File.WriteAllText(outputSourceFilepath, source.ToString());
        }
    }

    class Program
    {
        static void Main(string[] args)
        {
            if (!ProgramOptions.Parse(args))
                return;

            string text = File.ReadAllText(ProgramOptions.SourceFile);

            Console.WriteLine(" > Read input machine description file '{0}'", ProgramOptions.SourceFile);

            Tokeniser tokeniser = new Tokeniser(text);

            if (ProgramOptions.PrintTokens)
            {
                Console.WriteLine(" > Skipping parsing, dumping tokens to stdout");

                Token token = tokeniser.GetNext();

                while (token.Type != TokenType.EndOfFile)
                {
                    Console.WriteLine(token);
                    token = tokeniser.GetNext();
                }

                return;
            }

            SExpressionParser parser = new SExpressionParser();
            List<Node> nodes = parser.Parse(tokeniser);

            Console.WriteLine(" > Parsing complete, {0} root node(s)", nodes.Count);

            if (ProgramOptions.PrintAST)
            {
                Console.WriteLine(" > Skipping codegen, dumping AST to stdout");

                foreach (Node node in nodes)
                {
                    node.Print(new PrintingContext());
                    Console.WriteLine();
                }

                return;
            }

            string outputDirectory = ProgramOptions.OutputDirectory;

            if (!string.IsNullOrWhiteSpace(outputDirectory))
            {
                string machineName = Path.GetFileNameWithoutExtension(ProgramOptions.SourceFile);

                Console.WriteLine(" > Generating code to given directory '{0}'", outputDirectory);
                new MachineDescriptionCodeGenerator(nodes, machineName).Generate(outputDirectory);
            }
            else
            {
                Console.WriteLine(" > Invalid output directory given ('{0}'), skipping codegen", outputDirectory);
            }
        }
    }
}
