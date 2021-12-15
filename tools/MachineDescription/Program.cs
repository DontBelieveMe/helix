using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace MachineDescription
{
    class PrintingContext
    {
        public int CurrentIndent { get; private set; } = 0;
        private StringBuilder _target = null;

        public PrintingContext(StringBuilder sb)
        {
            _target = sb;
        }

        public PrintingContext() {  }

        public void IncreaseIndent(int count)
        {
            CurrentIndent += count;
        }

        public void DecreaseIndent(int count)
        {
            CurrentIndent -= count;

            if (CurrentIndent < 0)
                CurrentIndent = 0;
        }

        public void PrintIndentedLine(string line)
        {
            PrintIndent();

            if (_target != null)
                _target.AppendLine(line);
            else
                Console.WriteLine(line);
        }

        public void PrintIndent()
        {
            for (int i = 0; i < CurrentIndent; ++i)
            {
                if (_target != null)
                    _target.Append("\t");
                else
                    Console.Write("\t");
            }
        }
    }

    abstract class Node
    {
        public abstract void Print(PrintingContext context);
        public virtual bool HasValue(string value) { return false; }

        public Node Parent = null;
    }

    class Symbol : Node
    {
        public string Value;

        public override void Print(PrintingContext context)
        {
            Console.Write(Value);
        }

        public override bool HasValue(string value)
        {
            return Value == value;
        }
    }

    class String : Node
    {
        public string Value;

        public override void Print(PrintingContext context)
        {
            Console.Write("\"" + Value + "\"");
        }
    }

    class Number : Node
    {
        public int Value;

        public override void Print(PrintingContext context)
        {
            Console.Write(Value);
        }
    }

    class SExpression : Node
    {
        public List<Node> Children = new List<Node>();

        public virtual char GetLeftDelim() { return '('; }
        public virtual char GetRightDelim() { return ')'; }

        public int IndexOf(Node node)
        {
            return Children.IndexOf(node);
        }

        public T GetChildAs<T>(int index) where T: Node
        {
            if (index >= Children.Count)
                return null;

            return Children[index] as T;
        }

        public override void Print(PrintingContext context)
        {
            bool startOnNewLine = true;

            if (Parent != null)
            {
                if (Parent is SExpression)
                {
                    SExpression sp = (SExpression)Parent;

                    if (sp.Children[0] == this)
                    {
                        startOnNewLine = false;
                    }
                }

                if (startOnNewLine)
                {
                    Console.WriteLine();
                    context.IncreaseIndent(1);
                    context.PrintIndent();
                }
            }

            Console.Write(GetLeftDelim());
            for (int  i = 0; i < Children.Count; ++i)
            {
                Children[i].Print(context);

                if (i < Children.Count - 1)
                    Console.Write(" ");
            }
            Console.Write(GetRightDelim());

            if (startOnNewLine)
            {
                context.DecreaseIndent(1);
            }
        }
    }

    class Array : SExpression
    {
        public override char GetLeftDelim()
        {
            return '[';
        }

        public override char GetRightDelim()
        {
            return ']';
        }
    }

    enum TokenType
    {
        LParen,
        RParen,
        LBracket,
        RBracket,
        Number,
        String,
        Identifier,
        EndOfFile
    }

    struct Token
    {
        public TokenType Type;
        public string Value;

        public Token(TokenType type, string value)
        {
            Type = type;
            Value = value;
        }

        public override string ToString()
        {
            return string.Format("({0} '{1}')", Type, Value);
        }
    }

    class Tokeniser
    {
        private string _text;
        private int _index = 0;

        public Tokeniser(string text)
        {
            _text = text;
        }

        private char GetCurrentCharacter()
        {
            if (_index >= _text.Length)
                return '\0';

            return _text[_index];
        }

        private bool IsWhitespace(char c)
        {
            return c == ' ' || c == '\n' || c == '\t' || c == '\r';
        }

        private char NextChar()
        {
            if (_index < _text.Length)
                _index++;

            return GetCurrentCharacter();
        }

        private char SkipWhitespace()
        {
            char c = GetCurrentCharacter();

            while (IsWhitespace(c))
                c = NextChar();

            return c;
        }

        private char SkipToEndOfLine()
        {
            char c = GetCurrentCharacter();

            while (c != '\0' && c != '\n')
                c = NextChar();

            return c;
        }

        public Token GetNext()
        {
            char current = SkipWhitespace();

            if (current == ';')
            {
                SkipToEndOfLine();
                current = SkipWhitespace();
            }

            switch (current)
            {
                case '\0': return new Token(TokenType.EndOfFile, null);
                case '(': { NextChar(); return new Token(TokenType.LParen, "(");   }
                case ')': { NextChar(); return new Token(TokenType.RParen, ")");   }
                case '[': { NextChar(); return new Token(TokenType.LBracket, "["); }
                case ']': { NextChar(); return new Token(TokenType.RBracket, "]"); }
                case '"':
                    {
                        current = NextChar();
                        int startIndex = _index;
                        int length = 0;

                        while (current != '"')
                        {
                            length++;
                            current = NextChar();
                        }

                        string value = _text.Substring(startIndex, length);
                        current = NextChar();
                        return new Token(TokenType.String, value);
                    }

                default:
                    {
                        if (Char.IsLetter(current) || current == '-' || current == ':' || current == '_')
                        {
                            int startIndex = _index;
                            int length = 0;

                            while (Char.IsLetter(current) || current == '-' || current == ':' || current == '_' || Char.IsNumber(current))
                            {
                                length++;
                                current = NextChar();
                            }

                            string value = _text.Substring(startIndex, length);
                            return new Token(TokenType.Identifier, value);
                        }

                        if (Char.IsNumber(current))
                        {
                            int startIndex = _index;
                            int length = 0;

                            while (Char.IsNumber(current))
                            {
                                length++;
                                current = NextChar();
                            }

                            string value = _text.Substring(startIndex, length);
                            return new Token(TokenType.Number, value);
                        }

                        throw new SyntaxException("unknown character '" + current + "'");
                    }
            }
        }
    }

    class SyntaxException : Exception
    {
        public SyntaxException(string message) : base(message)
        {
        }
    }

    class Parser
    {
        private Token _currentToken;
        private Tokeniser _tokeniser;

        private void RequireCurrentTokenType(TokenType type)
        {
            if (_currentToken.Type != type)
            {
                throw new SyntaxException(string.Format("Expected token '{0}', instead got '{0}'", type, _currentToken.Type));
            }
        }

        private void AssumeCurrentToken(TokenType type)
        {
            RequireCurrentTokenType(type);
            _currentToken = _tokeniser.GetNext();
        }

        private Node ParseSExpr(Node parent)
        {
            AssumeCurrentToken(TokenType.LParen);

            SExpression expr = new SExpression();
            expr.Parent = parent;

            while (_currentToken.Type != TokenType.RParen)
            {
                expr.Children.Add(ParseNode(expr));
            }

            AssumeCurrentToken(TokenType.RParen);
            return expr;
        }

        private Node ParseArray(Node parent)
        {
            AssumeCurrentToken(TokenType.LBracket);

            Array expr = new Array();
            expr.Parent = parent;

            while (_currentToken.Type != TokenType.RBracket)
            {
                expr.Children.Add(ParseNode(expr));
            }

            AssumeCurrentToken(TokenType.RBracket);
            return expr;
        }

        private Node ParseNumber(Node parent)
        {
            Token tok = _currentToken;
            AssumeCurrentToken(TokenType.Number);
            return new Number() { Value = Convert.ToInt32(tok.Value), Parent = parent };
        }

        private Node ParseString(Node parent)
        {
            Token tok = _currentToken;
            AssumeCurrentToken(TokenType.String);
            return new String() { Value = tok.Value,Parent=parent };
        }

        private Node ParseSymbol(Node parent)
        {
            Token tok = _currentToken;
            AssumeCurrentToken(TokenType.Identifier);
            return new Symbol() { Value = tok.Value, Parent = parent };
        }

        private Node ParseNode(Node parent)
        {
            switch (_currentToken.Type)
            {
                case TokenType.LParen:
                    return ParseSExpr(parent);

                case TokenType.LBracket:
                    return ParseArray(parent);

                case TokenType.Number:
                    return ParseNumber(parent);

                case TokenType.String:
                    return ParseString(parent);

                case TokenType.Identifier:
                    return ParseSymbol(parent);

                default:
                    throw new SyntaxException("Unexpected token at the start of node");
            }
        }

        public List<Node> Parse(Tokeniser tokeniser)
        {
            List<Node> nodes = new List<Node>();

            _tokeniser = tokeniser;
            _currentToken = tokeniser.GetNext();

            while (_currentToken.Type != TokenType.EndOfFile)
            {
                nodes.Add(ParseNode(null));
            }

            return nodes;
        }
    }

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
            { "i32", "BuiltinTypes::GetInt32" }
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

                if (!outputString.Contains("{"))
                {
                    ctx.PrintIndentedLine("fprintf(file, \"\\t" + outputString + "\\n\");");
                }
                else
                {
                    List<string> formatArgs = new List<string>();

                    for (int i = 0; i < Operands.Count; ++i)
                    {
                        if (Operands[i] is MatchOperand)
                        {
                            MatchOperand mo = (MatchOperand)Operands[i];

                            ctx.PrintIndentedLine(string.Format("const std::string& a{0} = stringify_operand(insn.GetOperand({1}));", mo.OutputOperandIndex, i));
                            formatArgs.Add("a" + mo.OutputOperandIndex);
                        }
                    }

                    string args = string.Join(", ", formatArgs);

                    ctx.PrintIndentedLine(string.Format("const std::string as = fmt::format(\"\\t{0}\\n\", {1});", _parent.OutputFormat,args));
                    ctx.PrintIndentedLine("fprintf(file, \"%s\",as.c_str());");
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
            String name = node.GetChildAs<String>(1);
            SExpression template = node.GetChildAs<Array>(2).GetChildAs<SExpression>(0);
            String outputFormat = node.GetChildAs<String>(3);

            Instruction insn = new Instruction() { 
                Name = name.Value,
                OutputFormat = outputFormat.Value,
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

            headerFile.AppendLine("#pragma once");
            headerFile.AppendLine("");
            headerFile.AppendLine("#include <stdio.h>");

            headerFile.AppendLine("namespace Helix { class Instruction; }");

            // Namespace
            {
                headerFile.AppendLine("namespace Helix::ARMv7 {");
                ctx.IncreaseIndent(1);

                // Opcode enum
                {
                    ctx.PrintIndentedLine("enum Opcode {");
                    ctx.IncreaseIndent(1);

                    foreach (Instruction insn in _desc.Instructions)
                    {
                        ctx.PrintIndentedLine(Capitalise(insn.Name) + ",");
                    }

                    ctx.DecreaseIndent(1);
                    ctx.PrintIndentedLine("};");
                }

                // Function prototypes
                {
                    ctx.PrintIndentedLine("void Emit(FILE*, Instruction&);");
                }

                ctx.DecreaseIndent(1);
                headerFile.AppendLine("}");
            }
        }

        private void GenerateSource(StringBuilder sourceFile)
        {
            Console.WriteLine(" > Generating source file contents");

            PrintingContext ctx = new PrintingContext(sourceFile);

            ctx.PrintIndentedLine("#include \"arm-md.h\"");
            ctx.PrintIndentedLine("#include \"../src/instructions.h\"");
            ctx.PrintIndentedLine("#include \"../src/system.h\"");

            // Emit(FILE*,Instruction&)
            {
                ctx.PrintIndentedLine("void Helix::ARMv7::Emit(FILE* file, Instruction& insn)\n{");
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
            bool printTokens = false;
            bool printAST = false;

            string outputDirectory = "";

            foreach (string arg in args)
            {
                if (arg == "-print-tokens")
                    printTokens = true;
                else if (arg == "-print-ast")
                    printAST = true;
                else if (arg.StartsWith("-output="))
                    outputDirectory = arg.Split("=")[1];
            }

            string filepath = args[0];
            string text = File.ReadAllText(filepath);

            Console.WriteLine(" > Read input machine description file '{0}'", filepath);

            Tokeniser tokeniser = new Tokeniser(text);

            if (printTokens)
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

            Parser parser = new Parser();
            List<Node> nodes = parser.Parse(tokeniser);

            Console.WriteLine(" > Parsing complete, {0} root node(s)", nodes.Count);

            if (printAST)
            {
                Console.WriteLine(" > Skipping codegen, dumping AST to stdout");

                foreach (Node node in nodes)
                {
                    node.Print(new PrintingContext());
                    Console.WriteLine();
                }

                return;
            }

            if (!string.IsNullOrWhiteSpace(outputDirectory))
            {
                string machineName = Path.GetFileNameWithoutExtension(filepath);

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
