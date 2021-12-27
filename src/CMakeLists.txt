# C Frontend (Clang)
add_subdirectory(c)

# Compiler Driver
add_subdirectory(hxc)

# Target LibC
add_subdirectory(libc)

# Unit Tests (for HelixCore)
add_subdirectory(tests)

add_library(HelixCore
	basic-block.h
	basic-block.cpp
	instructions.h
	instructions.cpp
	value.h
	value.cpp
	types.h
	types.cpp
	hash.h
	helix.h
	helix.cpp
	print.h
	print.cpp
	function.h
	function.cpp
	system.h
	system.cpp
	options.h
	target-info.h
	target-info-armv7.h
	target-info-armv7.cpp
	module.h
	module.cpp
	pass-manager.h
	pass-manager.cpp
	lower.h
	lower.cpp
	profile.h
	match.cpp
	match.h
	regalloc.h
	regalloc.cpp
	validate.h
	validate.cpp

	options.def
	insns.def

	intrusive-list.h
	iterator-range.h

	${CMAKE_CURRENT_BINARY_DIR}/arm-md.h
	${CMAKE_CURRENT_BINARY_DIR}/arm-md.cpp
)

target_link_libraries(HelixCore PUBLIC spdlog::spdlog Tracy::TracyClient)

# Code Generation for the *.md files (machine descriptions)
if (WIN32)
	set(MachineDescriptionTool ${CMAKE_SOURCE_DIR}/tools/MachineDescription/bin/Release/net5.0/MachineDescription.exe)
	
	add_custom_command(
		COMMAND ${MachineDescriptionTool} ${CMAKE_SOURCE_DIR}/src/arm.md -output=${CMAKE_CURRENT_BINARY_DIR}
		DEPENDS arm.md ${MachineDescriptionTool}
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/arm-md.cpp ${CMAKE_CURRENT_BINARY_DIR}/arm-md.h
		COMMENT "Generating ARM machine description" 
	)
endif()

# Compile Time Configuration

set (CONFIG_LIBC_INCLUDE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/libc/include")
configure_file(helix-config.in.h helix-config.h)

if (MSVC)
	target_link_options(HelixCore PRIVATE /DEBUG)
	target_compile_options(HelixCore PRIVATE /wd26812 /W4 /Zi)
endif()