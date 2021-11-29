#pragma once

#include "instructions.h"
#include "basic-block.h"
#include "function.h"
#include "system.h"
#include "options.h"

#include <unordered_map>

namespace Helix
{
	class TextOutputStream
	{
	public:
		//////////////////////////////////////////////////////////////////////////
		// ASCII Colour Codes
		//////////////////////////////////////////////////////////////////////////

		static constexpr const char* kColour_Red = "\033[0;31m";
		static constexpr const char* kColour_Black = "\033[0;31m";
		static constexpr const char* kColour_Green = "\033[0;32m";
		static constexpr const char* kColour_Yellow = "\033[0;33m";
		static constexpr const char* kColour_Blue = "\033[0;34m";
		static constexpr const char* kColour_Purple = "\033[0;35m";
		static constexpr const char* kColour_Cyan = "\033[0;36m";
		static constexpr const char* kColour_White = "\033[0;37m";

		/// Construct this TextOutputStream to write to a FILE handle
		/// (e.g. commonly stdout or stderr, but can also be a arbitrary file).
		TextOutputStream(FILE* file)
			: m_File(file) { }

		/// Construct this TextOutputStream to write to a string buffer of the given
		/// size. Trying to write any text beyond the buffer size is a safe no-op.
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

		/// Set the output colour for when printing to stdout or stderr.
		/// For any other files or outputting to string this does not do anything.
		void SetColour(const char* code)
		{
			if (Options::GetDisableTerminalColouring())
				return;

			if (m_File == stdout || m_File == stderr)
				fprintf(m_File, "%s", code);
		}

		/// Reset the output colour. This only applies when outputting to stdout or
		/// stderr.
		void ResetColour()
		{
			if (Options::GetDisableTerminalColouring())
				return;

			if (m_File == stdout || m_File == stderr)
				fprintf(m_File, "\033[0m");
		}

	private:
		FILE*  m_File          = nullptr;
		char*  m_StringBuf     = nullptr;
		size_t m_StringBufSize = 0;
		size_t m_StringBufHead = 0;
	};

	class SlotTracker
	{
	public:
		size_t GetValueSlot(const Value* value)
		{
			auto it = m_ValueSlots.find(value);

			if (it == m_ValueSlots.end()) {
				size_t size = m_ValueSlots.size();
				m_ValueSlots[value] = size;
				return size;
			}

			return it->second;
		}

		size_t GetBasicBlockSlot(const BasicBlock* bb)
		{
			auto it = m_BlockSlots.find(bb);

			if (it == m_BlockSlots.end()) {
				size_t size = m_BlockSlots.size();
				m_BlockSlots[bb] = size;
				return size;
			}

			return it->second;
		}

	private:
		std::unordered_map<const Value*, size_t>      m_ValueSlots;
		std::unordered_map<const BasicBlock*, size_t> m_BlockSlots;
	};

	/// For a given opcode return a statically allocated string representing
	/// the name of the instruction. If the opcode is unknown "?" is returned.
	const char* GetOpcodeName(Opcode opcode);

	/// For a given type ID return a statically allocated string representing that type
	/// (e.g. the name of the type). If the type is unknown "?" is returned.
	const char* GetTypeName(const Helix::Type* type);

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
