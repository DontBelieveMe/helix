add_subdirectory(external/antlr4/runtime/Cpp)
set_property(TARGET antlr4_static antlr4_shared PROPERTY FOLDER "External")
include_directories(external/antlr4/runtime/Cpp/runtime/src)
