using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

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

        public PrintingContext() { }

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

        public void PrintIndentedLineThenIndent(string line, int indentCount = 1)
        {
            PrintIndentedLine(line);
            IncreaseIndent(indentCount);
        }

        public void Newline()
        {
            _target.AppendLine();
        }

        public void PrintIndentedLineAfterUnindent(string line, int indentCount = 1)
        {
            DecreaseIndent(indentCount);
            PrintIndentedLine(line);
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
}
