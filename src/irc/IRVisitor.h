
// Generated from C:\helix\tools\\..\src\irc\IR.g4 by ANTLR 4.9.3

#pragma once


#include "antlr4-runtime.h"
#include "IRParser.h"


namespace Helix::Frontend::IR {

/**
 * This class defines an abstract visitor for a parse tree
 * produced by IRParser.
 */
class  IRVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  /**
   * Visit parse trees produced by IRParser.
   */
    virtual antlrcpp::Any visitModule(IRParser::ModuleContext *context) = 0;

    virtual antlrcpp::Any visitDecl(IRParser::DeclContext *context) = 0;

    virtual antlrcpp::Any visitFunction(IRParser::FunctionContext *context) = 0;


};

}  // namespace Helix::Frontend::IR
