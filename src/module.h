#pragma once

#include "function.h"
#include "types.h"
#include "iterator-range.h"

#include <vector>

namespace Helix
{
    class Module
    {
    private:
        using FunctionList = std::vector<Function*>;
        using StructList = std::vector<const StructType*>;
        using GlobalsList = std::vector<GlobalVariable*>;

    public:
        using function_iterator       = FunctionList::iterator;
        using const_function_iterator = FunctionList::const_iterator;
        using struct_iterator         = StructList::iterator;
        using const_struct_iterator   = StructList::const_iterator;
        using globals_iterator        = GlobalsList::iterator;
        using const_globals_iterator  = GlobalsList::const_iterator;

        size_t GetCountFunctions() const { return m_Functions.size(); }
        size_t GetCountStructs() const { return m_Structs.size(); }
        size_t GetCountGlobalVars() const { return m_GlobalVariables.size(); }

        void RegisterFunction(Function* fn)
        {
            m_Functions.push_back(fn);
        }

        void RegisterStruct(const StructType* ty)
        {
            m_Structs.push_back(ty);
        }

        void RegisterGlobalVariable(GlobalVariable* gvar)
        {
            m_GlobalVariables.push_back(gvar);
        }

        function_iterator       functions_begin()       { return m_Functions.begin(); }
        function_iterator       functions_end()         { return m_Functions.end();   }
        const_function_iterator functions_begin() const { return m_Functions.begin(); }
        const_function_iterator functions_end()   const { return m_Functions.end();   }

        struct_iterator         structs_begin()         { return m_Structs.begin();   }
        struct_iterator         structs_end()           { return m_Structs.end();     }
        const_struct_iterator   structs_begin()   const { return m_Structs.begin();   }
        const_struct_iterator   structs_end()     const { return m_Structs.end();     }

        globals_iterator        globals_begin()         { return m_GlobalVariables.begin();   }
        globals_iterator        globals_end()           { return m_GlobalVariables.end();     }
        const_globals_iterator  globals_begin()   const { return m_GlobalVariables.begin();   }
        const_globals_iterator  globals_end()     const { return m_GlobalVariables.end();     }


        iterator_range<const_function_iterator> functions() const { return iterator_range(m_Functions.begin(), m_Functions.end()); }
        iterator_range<const_globals_iterator>  globals()   const { return iterator_range(m_GlobalVariables.begin(), m_GlobalVariables.end()); }
        iterator_range<const_struct_iterator>   structs()   const { return iterator_range(m_Structs.begin(), m_Structs.end()); }

        iterator_range<function_iterator>       functions()       { return iterator_range(m_Functions.begin(), m_Functions.end()); }
        iterator_range<globals_iterator>        globals()         { return iterator_range(m_GlobalVariables.begin(), m_GlobalVariables.end()); }
        iterator_range<struct_iterator>         structs()         { return iterator_range(m_Structs.begin(), m_Structs.end()); }

    private:
        FunctionList m_Functions;
        StructList   m_Structs;
        GlobalsList  m_GlobalVariables;
    };

    inline Module* CreateModule()
    {
        return new Module();
    }
}