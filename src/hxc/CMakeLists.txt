add_executable(HelixCompiler
	main.cpp
)

target_link_libraries(HelixCompiler HelixCore HelixClangFrontend)
set_target_properties(HelixCompiler PROPERTIES OUTPUT_NAME hxc)

if (MSVC)
	target_link_options(HelixCompiler PRIVATE /DEBUG)
	target_compile_options(HelixCompiler PRIVATE /wd26812 /W4 /Zi)
endif()
