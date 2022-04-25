
// Generated from C:\helix\tools\\..\src\irc\IR.g4 by ANTLR 4.9.3

#pragma once


#include "antlr4-runtime.h"
#include "IRVisitor.h"


namespace Helix::Frontend::IR {

/**
 * This class provides an empty implementation of IRVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  IRBaseVisitor : public IRVisitor {
public:

  virtual antlrcpp::Any visitModule(IRParser::ModuleContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitDecl(IRParser::DeclContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitFunction(IRParser::FunctionContext *ctx) override {
    return visitChildren(ctx);
  }


};

}  // namespace Helix::Frontend::IR
