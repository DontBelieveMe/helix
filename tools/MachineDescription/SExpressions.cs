using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MachineDescription
{
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

        public T GetChildAs<T>(int index) where T : Node
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
            for (int i = 0; i < Children.Count; ++i)
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
}
