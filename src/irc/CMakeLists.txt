add_library(HelixIRFrontend
	ir-frontend.cpp
	ir-frontend.h

	IR.interp
	IR.tokens
	IRBaseVisitor.cpp
	IRBaseVisitor.h
	IRLexer.h
	IRLexer.cpp
	IRLexer.interp
	IRLexer.tokens
	IRVisitor.cpp
	IRVisitor.h
	IRParser.h
	IRParser.cpp
)

if (MSVC)
	target_link_options(HelixIRFrontend PRIVATE /ignore:4099 /DEBUG)
	target_compile_options(HelixIRFrontend PRIVATE /wd26812 /W4 /Zi)
endif()

target_link_libraries(HelixIRFrontend HelixCore antlr4_shared)
