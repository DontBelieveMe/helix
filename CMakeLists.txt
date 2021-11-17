cmake_minimum_required(VERSION 3.0)
project(Helix)

include(cmake/clang.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_executable(helix
	src/main.cpp
	src/clang_frontend.cpp
	src/frontend.h
	src/basic_block.h
	src/basic_block.cpp
	src/instructions.h
	src/instructions.cpp
	src/intrusive_list.h
)

target_link_libraries(helix Clang::LibTooling)
target_link_options(helix PRIVATE /ignore:4099)
target_compile_options(helix PRIVATE /wd26812 /W4)

add_executable(unit-tests
	src/tests/test_intrusive_list.cpp
	src/tests/main.cpp
	src/tests/catch.hpp
)