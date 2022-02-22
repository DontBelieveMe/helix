add_library(HelixClangFrontend
	clang-frontend.cpp
	clang-frontend.h
)

if (MSVC)
	target_link_options(HelixClangFrontend PRIVATE /ignore:4099 /DEBUG)
	target_compile_options(HelixClangFrontend PRIVATE /wd26812 /W4 /Zi)
endif()

target_link_libraries(HelixClangFrontend HelixCore Clang::LibTooling)
