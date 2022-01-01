using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MachineDescription
{
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

            while (current == ';')
            {
                SkipToEndOfLine();
                current = SkipWhitespace();
            }

            switch (current)
            {
                case '\0': return new Token(TokenType.EndOfFile, null);
                case '(': { NextChar(); return new Token(TokenType.LParen, "("); }
                case ')': { NextChar(); return new Token(TokenType.RParen, ")"); }
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

    class SExpressionParser
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
            return new String() { Value = tok.Value, Parent = parent };
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
}
