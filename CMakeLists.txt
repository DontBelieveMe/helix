cmake_minimum_required(VERSION 3.0)
project(Helix)

include(cmake/clang.cmake)
include(cmake/tracy.cmake)
include(cmake/spdlog.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(LibHelix
	src/basic-block.h
	src/basic-block.cpp
	src/instructions.h
	src/instructions.cpp
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
	src/target-info.h
	src/target-info-armv7.h
	src/target-info-armv7.cpp
	src/module.h
	src/pass-manager.h
	src/pass-manager.cpp
	src/lower.h
	src/lower.cpp
	src/profile.h
	src/match.cpp
	src/match.h
	src/regalloc.h
	src/regalloc.cpp

	src/options.def
	src/insns.def

	src/intrusive-list.h
	src/iterator-range.h

	${CMAKE_CURRENT_BINARY_DIR}/arm-md.h
	${CMAKE_CURRENT_BINARY_DIR}/arm-md.cpp
)

target_link_libraries(LibHelix PUBLIC spdlog::spdlog Tracy::TracyClient)

set(MachineDescriptionTool ${CMAKE_CURRENT_LIST_DIR}/tools/MachineDescription/bin/Release/net5.0/MachineDescription.exe)

add_custom_command(
	COMMAND ${MachineDescriptionTool} ${CMAKE_CURRENT_LIST_DIR}/src/arm.md -output=${CMAKE_CURRENT_BINARY_DIR}
	DEPENDS src/arm.md
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/arm-md.cpp ${CMAKE_CURRENT_BINARY_DIR}/arm-md.h
	COMMENT "Generating ARM machine description" 
)

add_executable(Helix
	src/main.cpp
	src/clang-frontend.cpp
	src/frontend.h
)

target_link_libraries(Helix Clang::LibTooling LibHelix)
target_link_options(Helix PRIVATE /ignore:4099 /DEBUG)
target_link_options(LibHelix PRIVATE /DEBUG)

target_compile_options(Helix PRIVATE /wd26812 /W4 /Zi)
target_compile_options(LibHelix PRIVATE /wd26812 /W4 /Zi)

add_executable(LibHelixTests
	src/tests/test_intrusive_list.cpp
	src/tests/test_bytecode.cpp
	src/tests/test_value.cpp
	src/tests/test_print.cpp
	src/tests/test_function.cpp
	src/tests/main.cpp
	src/tests/catch.hpp
)

target_link_libraries(LibHelixTests LibHelix)

set_target_properties(LibHelixTests PROPERTIES FOLDER "Tests")

set_target_properties(LibHelixTests PROPERTIES OUTPUT_NAME libhelix-tests)
set_target_properties(Helix PROPERTIES OUTPUT_NAME helix)

