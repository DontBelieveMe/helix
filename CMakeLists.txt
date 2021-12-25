cmake_minimum_required(VERSION 3.0)
project(Helix)

# External Dependency Configuration
include(cmake/clang.cmake)
include(cmake/tracy.cmake)
include(cmake/spdlog.cmake)

# The compiler should be C++17 standard compatible as much as possible
# (no extensions for portability)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Useful for IDE environments that enable sorting projects/targets into
# "folders". Mostly just for Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Automatically add the current source directory (not as nessesary)
# and the build directory (nessesary for generated files) to
# all project include paths list.
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# The magic
add_subdirectory(src)


# Setup code to allow doing code coverage testing (only works under linux
# currently)
set(EnableCodeCoverage OFF)

if (EnableCodeCoverage)
	include(CTest)
	include(cmake/coverage.cmake)
	
	enable_testing()

	target_compile_options(HelixCore PRIVATE -O0)
	target_compile_options(HelixCoreTests PRIVATE -O0)

	append_coverage_compiler_flags()

	setup_target_for_coverage_lcov(
		NAME coverage
		EXECUTABLE HelixCoreTests
		DEPENDENCIES HelixCore HelixCoreTests
		BASE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/src")
endif()
