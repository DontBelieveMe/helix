/**
 * @file module.h
 * @author Barney Wilks
 *
 * Defines a IR module, the root container for all the IR
 */

#pragma once

/* Internal Project Includes */
#include "function.h"
#include "types.h"
#include "iterator-range.h"

/* C++ Standard Library Includes */
#include <vector>

namespace Helix
{
	class Module
	{
	private:
		using FunctionList = std::vector<Function*>;
		using StructList   = std::vector<const StructType*>;
		using GlobalsList  = std::vector<GlobalVariable*>;

	public:
		Module(const std::string& inputSourceFile);

		using function_iterator       = FunctionList::iterator;
		using const_function_iterator = FunctionList::const_iterator;
		using struct_iterator         = StructList::iterator;
		using const_struct_iterator   = StructList::const_iterator;
		using globals_iterator        = GlobalsList::iterator;
		using const_globals_iterator  = GlobalsList::const_iterator;

		using function_const_iterator_range = iterator_range<const_function_iterator>;
		using function_iterator_range       = iterator_range<function_iterator>;
		using struct_const_iterator_range   = iterator_range<const_struct_iterator>;
		using struct_iterator_range         = iterator_range<struct_iterator>;
		using globals_const_iterator_range  = iterator_range<const_globals_iterator>;
		using globals_iterator_range        = iterator_range<globals_iterator>;

		size_t      GetCountFunctions()  const { return m_Functions.size(); }
		size_t      GetCountStructs()    const { return m_Structs.size(); }
		size_t      GetCountGlobalVars() const { return m_GlobalVariables.size(); }
		std::string GetInputSourceFile() const { return m_InputSourceFile; }

		void RegisterFunction(Function* fn);
		void RegisterStruct(const StructType* ty);
		void RegisterGlobalVariable(GlobalVariable* gvar);

		Function* FindFunctionByName(const std::string& name) const;

		void DumpControlFlowGraphToFile(const std::string& filepath);

		function_iterator       functions_begin()       { return m_Functions.begin(); }
		function_iterator       functions_end()         { return m_Functions.end(); }
		const_function_iterator functions_begin() const { return m_Functions.begin(); }
		const_function_iterator functions_end()   const { return m_Functions.end(); }

		struct_iterator         structs_begin()         { return m_Structs.begin(); }
		struct_iterator         structs_end()           { return m_Structs.end(); }
		const_struct_iterator   structs_begin()   const { return m_Structs.begin(); }
		const_struct_iterator   structs_end()     const { return m_Structs.end(); }

		globals_iterator        globals_begin()         { return m_GlobalVariables.begin(); }
		globals_iterator        globals_end()           { return m_GlobalVariables.end(); }
		const_globals_iterator  globals_begin()   const { return m_GlobalVariables.begin(); }
		const_globals_iterator  globals_end()     const { return m_GlobalVariables.end(); }

		function_const_iterator_range functions() const { return function_const_iterator_range(m_Functions); }
		function_iterator_range       functions()       { return function_iterator_range(m_Functions); }

		struct_const_iterator_range   structs()   const { return struct_const_iterator_range(m_Structs); }
		struct_iterator_range         structs()         { return struct_iterator_range(m_Structs); }

		globals_const_iterator_range  globals()   const { return globals_const_iterator_range(m_GlobalVariables); }
		globals_iterator_range        globals()         { return globals_iterator_range(m_GlobalVariables); }

	private:
		FunctionList m_Functions;
		StructList   m_Structs;
		GlobalsList  m_GlobalVariables;
		std::string  m_InputSourceFile;
	};

	Module* CreateModule(const std::string& inputSourceFile);
}
