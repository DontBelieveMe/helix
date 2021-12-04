cmake_minimum_required(VERSION 3.0)
project(Helix)

include(cmake/clang.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(external/spdlog)

find_package(Threads REQUIRED)

add_library(TracyClient external/tracy/TracyClient.cpp)

target_compile_features(TracyClient PUBLIC cxx_std_11)
target_include_directories(TracyClient PUBLIC external/tracy)

target_link_libraries(
    TracyClient
    PUBLIC
        Threads::Threads
        ${CMAKE_DL_LIBS}
)

macro(set_option option help value)
    if(${option})
        message(STATUS "${option}: ON")
        target_compile_definitions(TracyClient PUBLIC ${option})
    else()
        message(STATUS "${option}: OFF")
    endif()
endmacro()

set_option(TRACY_ENABLE "Enable profiling" OFF)
set_option(TRACY_ON_DEMAND "On-demand profiling" OFF)
set_option(TRACY_CALLSTACK "Collect call stacks" OFF)
set_option(TRACY_ONLY_LOCALHOST "Only listen on the localhost interface" OFF)
set_option(TRACY_NO_BROADCAST "Disable client discovery by broadcast to local network" OFF)
set_option(TRACY_NO_CODE_TRANSFER "Disable collection of source code" OFF)
set_option(TRACY_NO_CONTEXT_SWITCH "Disable capture of context switches" OFF)
set_option(TRACY_NO_EXIT "Client executable does not exit until all profile data is sent to server" OFF)
set_option(TRACY_NO_FRAME_IMAGE "Disable capture of frame images" OFF)
set_option(TRACE_NO_SAMPLING "Disable call stack sampling" OFF)
set_option(TRACY_NO_VERIFY "Disable zone validation for C API" OFF)
set_option(TRACY_NO_VSYNC_CAPTURE "Disable capture of hardware Vsync events" OFF)

add_library(libhelix
	src/basic-block.h
	src/basic-block.cpp
	src/instructions.h
	src/instructions.cpp
	src/intrusive-list.h
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
	src/system.h
	src/system.cpp
	src/options.h
	src/options.cpp
	src/target-info.h
	src/target-info-armv7.h
	src/target-info-armv7.cpp
)

target_link_libraries(libhelix PUBLIC spdlog::spdlog TracyClient)

add_executable(helix
	src/main.cpp
	src/clang-frontend.cpp
	src/frontend.h
)

target_link_libraries(helix Clang::LibTooling libhelix)
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

target_link_libraries(unit-tests libhelix)
