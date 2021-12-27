find_package(Threads REQUIRED)

if (EXISTS "${CMAKE_SOURCE_DIR}/extras/tracy/")
    set (HAS_TRACY TRUE)
else()
    set (HAS_TRACY FALSE)
    add_library(TracyClient INTERFACE)
    add_library(Tracy::TracyClient ALIAS TracyClient)
    return()
endif()

add_library(TracyClient ${CMAKE_SOURCE_DIR}/extras/tracy/TracyClient.cpp)

target_compile_features(TracyClient PUBLIC cxx_std_11)
target_include_directories(TracyClient PUBLIC ${CMAKE_SOURCE_DIR}/extras/tracy)

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

target_compile_definitions(TracyClient PUBLIC -DHAS_TRACY)

add_library(Tracy::TracyClient ALIAS TracyClient)

if (ENABLE_PROFILING)
    set(TRACY_ENABLE ON)
    set(TRACY_NO_EXIT ON)
endif()

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


set_property(TARGET TracyClient PROPERTY FOLDER "External")
