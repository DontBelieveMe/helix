cmake_minimum_required(VERSION 3.0)
project(Helix)

include(cmake/clang.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_library(helix-ir
	src/basic_block.h
	src/basic_block.cpp
	src/instructions.h
	src/instructions.cpp
	src/intrusive_list.h
	src/value.h
	src/value.cpp
	src/types.h
	src/types.cpp
	src/hash.h
	src/helix.h
	src/helix.cpp
	src/print.h
	src/print.cpp
	src/function.h
	src/function.cpp
	src/core.h
)

add_executable(helix
	src/main.cpp
	src/clang_frontend.cpp
	src/frontend.h
)

target_link_libraries(helix Clang::LibTooling helix-ir)
target_link_options(helix PRIVATE /ignore:4099)
target_compile_options(helix PRIVATE /wd26812 /W4)

add_executable(unit-tests
	src/tests/test_intrusive_list.cpp
	src/tests/test_bytecode.cpp
	src/tests/test_value.cpp
	src/tests/test_print.cpp
	src/tests/test_function.cpp
	src/tests/main.cpp
	src/tests/catch.hpp
)

target_link_libraries(unit-tests helix-ir)
