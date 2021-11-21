#pragma once

#include "instructions.h"
#include "basic_block.h"
#include "function.h"

namespace Helix
{
	class TextOutputStream
	{
	public:
		TextOutputStream(FILE* file)
			: m_File(file) { }

		TextOutputStream(char* buf, size_t bufsize)
			: m_StringBuf(buf), m_StringBufSize(bufsize) { }

		template <typename... Args>
		void Write(const char* fmt, const Args&... args)
		{
			if (m_File) {
				fprintf(m_File, fmt, args...);
			}

			if (m_StringBuf && m_StringBufHead + 1 < m_StringBufSize) {
				char* buf = m_StringBuf + m_StringBufHead;
				size_t size = m_StringBufSize - m_StringBufHead;

				const size_t nPrinted = snprintf(buf, size, fmt, args...);
				m_StringBufHead += nPrinted;
			} 
		}

	private:
		FILE*  m_File          = nullptr;
		char*  m_StringBuf     = nullptr;
		size_t m_StringBufSize = 0;
		size_t m_StringBufHead = 0;
	};

	const char* GetOpcodeName(Opcode opcode);

	void Print(TextOutputStream& out, const Instruction& insn);
	void Print(TextOutputStream& out, const Value& value);
	void Print(TextOutputStream& out, const BasicBlock& bb);
	void Print(TextOutputStream& out, const Function& fn);

	/// Utility function for printing any 'Print(...)' overloaded type to stdout
	template <typename T>
	inline void DebugDump(const T& value)
	{
		TextOutputStream out(stdout);
		Print(out, value);
	}
}
