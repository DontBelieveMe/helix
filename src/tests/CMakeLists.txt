add_executable(HelixCoreTests
	test-intrusive-list.cpp
	test-bytecode.cpp
	test-value.cpp
	test-print.cpp
	test-function.cpp
	test-constant-int.cpp
	test-instructions.cpp
	test-instruction-index.cpp
	main.cpp
	catch.hpp
)

# FIXME(bwilks): Need to link against HelixClangFrontend since that is where
#                the CLI input options are implemented... not great :(

target_link_libraries(HelixCoreTests HelixCore HelixClangFrontend)
set_target_properties(HelixCoreTests PROPERTIES OUTPUT_NAME libhelix-tests)

set_target_properties(HelixCoreTests PROPERTIES FOLDER "Tests")

if (MSVC)
	target_link_options(HelixCoreTests PRIVATE /DEBUG)
	target_compile_options(HelixCoreTests PRIVATE /wd26812 /W4 /Zi)
endif()
