#pragma once

#include "function.h"
#include "types.h"

#include <vector>

namespace Helix
{
    class Module
    {
    private:
        using FunctionList = std::vector<Function*>;
        using StructList = std::vector<const StructType*>;

    public:
        using function_iterator       = FunctionList::iterator;
        using const_function_iterator = FunctionList::const_iterator;
        using struct_iterator         = StructList::iterator;
        using const_struct_iterator   = StructList::const_iterator;

        void RegisterFunction(Function* fn)
        {
            m_Functions.push_back(fn);
        }

        void RegisterStruct(const StructType* ty)
        {
            m_Structs.push_back(ty);
        }

        function_iterator       functions_begin()       { return m_Functions.begin(); }
        function_iterator       functions_end()         { return m_Functions.end();   }
        const_function_iterator functions_begin() const { return m_Functions.begin(); }
        const_function_iterator functions_end()   const { return m_Functions.end();   }

        struct_iterator         structs_begin()         { return m_Structs.begin();   }
        struct_iterator         structs_end()           { return m_Structs.end();     }
        const_struct_iterator   structs_begin()   const { return m_Structs.begin();   }
        const_struct_iterator   structs_end()     const { return m_Structs.end();     }

    private:
        FunctionList m_Functions;
        StructList   m_Structs;
    };

    inline Module* CreateModule()
    {
        return new Module();
    }
}