cmake_minimum_required(VERSION 3.0)
project(Helix)

include(cmake/clang.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(helix
	src/main.cpp
	src/clang_frontend.cpp
	src/frontend.h
)

target_link_libraries(helix Clang::LibTooling)
target_link_options(helix PRIVATE /ignore:4099)
