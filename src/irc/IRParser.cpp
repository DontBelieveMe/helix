
// Generated from C:\helix\tools\\..\src\irc\IR.g4 by ANTLR 4.9.3


#include "IRVisitor.h"

#include "IRParser.h"


using namespace antlrcpp;
using namespace Helix::Frontend::IR;
using namespace antlr4;

IRParser::IRParser(TokenStream *input) : Parser(input) {
  _interpreter = new atn::ParserATNSimulator(this, _atn, _decisionToDFA, _sharedContextCache);
}

IRParser::~IRParser() {
  delete _interpreter;
}

std::string IRParser::getGrammarFileName() const {
  return "IR.g4";
}

const std::vector<std::string>& IRParser::getRuleNames() const {
  return _ruleNames;
}

dfa::Vocabulary& IRParser::getVocabulary() const {
  return _vocabulary;
}


//----------------- ModuleContext ------------------------------------------------------------------

IRParser::ModuleContext::ModuleContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* IRParser::ModuleContext::EOF() {
  return getToken(IRParser::EOF, 0);
}

std::vector<IRParser::DeclContext *> IRParser::ModuleContext::decl() {
  return getRuleContexts<IRParser::DeclContext>();
}

IRParser::DeclContext* IRParser::ModuleContext::decl(size_t i) {
  return getRuleContext<IRParser::DeclContext>(i);
}


size_t IRParser::ModuleContext::getRuleIndex() const {
  return IRParser::RuleModule;
}


antlrcpp::Any IRParser::ModuleContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IRVisitor*>(visitor))
    return parserVisitor->visitModule(this);
  else
    return visitor->visitChildren(this);
}

IRParser::ModuleContext* IRParser::module() {
  ModuleContext *_localctx = _tracker.createInstance<ModuleContext>(_ctx, getState());
  enterRule(_localctx, 0, IRParser::RuleModule);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(7); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(6);
      decl();
      setState(9); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == IRParser::FUNCTION);
    setState(11);
    match(IRParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- DeclContext ------------------------------------------------------------------

IRParser::DeclContext::DeclContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

IRParser::FunctionContext* IRParser::DeclContext::function() {
  return getRuleContext<IRParser::FunctionContext>(0);
}


size_t IRParser::DeclContext::getRuleIndex() const {
  return IRParser::RuleDecl;
}


antlrcpp::Any IRParser::DeclContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IRVisitor*>(visitor))
    return parserVisitor->visitDecl(this);
  else
    return visitor->visitChildren(this);
}

IRParser::DeclContext* IRParser::decl() {
  DeclContext *_localctx = _tracker.createInstance<DeclContext>(_ctx, getState());
  enterRule(_localctx, 2, IRParser::RuleDecl);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(13);
    function();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- FunctionContext ------------------------------------------------------------------

IRParser::FunctionContext::FunctionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* IRParser::FunctionContext::FUNCTION() {
  return getToken(IRParser::FUNCTION, 0);
}


size_t IRParser::FunctionContext::getRuleIndex() const {
  return IRParser::RuleFunction;
}


antlrcpp::Any IRParser::FunctionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IRVisitor*>(visitor))
    return parserVisitor->visitFunction(this);
  else
    return visitor->visitChildren(this);
}

IRParser::FunctionContext* IRParser::function() {
  FunctionContext *_localctx = _tracker.createInstance<FunctionContext>(_ctx, getState());
  enterRule(_localctx, 4, IRParser::RuleFunction);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(15);
    match(IRParser::FUNCTION);
    setState(16);
    match(IRParser::T__0);
    setState(17);
    match(IRParser::T__1);
    setState(18);
    match(IRParser::T__2);
    setState(19);
    match(IRParser::T__3);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

// Static vars and initialization.
std::vector<dfa::DFA> IRParser::_decisionToDFA;
atn::PredictionContextCache IRParser::_sharedContextCache;

// We own the ATN which in turn owns the ATN states.
atn::ATN IRParser::_atn;
std::vector<uint16_t> IRParser::_serializedATN;

std::vector<std::string> IRParser::_ruleNames = {
  "module", "decl", "function"
};

std::vector<std::string> IRParser::_literalNames = {
  "", "'('", "')'", "'{'", "'}'", "'function'", "'struct'"
};

std::vector<std::string> IRParser::_symbolicNames = {
  "", "", "", "", "", "FUNCTION", "STRUCT"
};

dfa::Vocabulary IRParser::_vocabulary(_literalNames, _symbolicNames);

std::vector<std::string> IRParser::_tokenNames;

IRParser::Initializer::Initializer() {
	for (size_t i = 0; i < _symbolicNames.size(); ++i) {
		std::string name = _vocabulary.getLiteralName(i);
		if (name.empty()) {
			name = _vocabulary.getSymbolicName(i);
		}

		if (name.empty()) {
			_tokenNames.push_back("<INVALID>");
		} else {
      _tokenNames.push_back(name);
    }
	}

  static const uint16_t serializedATNSegment0[] = {
    0x3, 0x608b, 0xa72a, 0x8133, 0xb9ed, 0x417c, 0x3be7, 0x7786, 0x5964, 
       0x3, 0x8, 0x18, 0x4, 0x2, 0x9, 0x2, 0x4, 0x3, 0x9, 0x3, 0x4, 0x4, 
       0x9, 0x4, 0x3, 0x2, 0x6, 0x2, 0xa, 0xa, 0x2, 0xd, 0x2, 0xe, 0x2, 
       0xb, 0x3, 0x2, 0x3, 0x2, 0x3, 0x3, 0x3, 0x3, 0x3, 0x4, 0x3, 0x4, 
       0x3, 0x4, 0x3, 0x4, 0x3, 0x4, 0x3, 0x4, 0x3, 0x4, 0x2, 0x2, 0x5, 
       0x2, 0x4, 0x6, 0x2, 0x2, 0x2, 0x15, 0x2, 0x9, 0x3, 0x2, 0x2, 0x2, 
       0x4, 0xf, 0x3, 0x2, 0x2, 0x2, 0x6, 0x11, 0x3, 0x2, 0x2, 0x2, 0x8, 
       0xa, 0x5, 0x4, 0x3, 0x2, 0x9, 0x8, 0x3, 0x2, 0x2, 0x2, 0xa, 0xb, 
       0x3, 0x2, 0x2, 0x2, 0xb, 0x9, 0x3, 0x2, 0x2, 0x2, 0xb, 0xc, 0x3, 
       0x2, 0x2, 0x2, 0xc, 0xd, 0x3, 0x2, 0x2, 0x2, 0xd, 0xe, 0x7, 0x2, 
       0x2, 0x3, 0xe, 0x3, 0x3, 0x2, 0x2, 0x2, 0xf, 0x10, 0x5, 0x6, 0x4, 
       0x2, 0x10, 0x5, 0x3, 0x2, 0x2, 0x2, 0x11, 0x12, 0x7, 0x7, 0x2, 0x2, 
       0x12, 0x13, 0x7, 0x3, 0x2, 0x2, 0x13, 0x14, 0x7, 0x4, 0x2, 0x2, 0x14, 
       0x15, 0x7, 0x5, 0x2, 0x2, 0x15, 0x16, 0x7, 0x6, 0x2, 0x2, 0x16, 0x7, 
       0x3, 0x2, 0x2, 0x2, 0x3, 0xb, 
  };

  _serializedATN.insert(_serializedATN.end(), serializedATNSegment0,
    serializedATNSegment0 + sizeof(serializedATNSegment0) / sizeof(serializedATNSegment0[0]));


  atn::ATNDeserializer deserializer;
  _atn = deserializer.deserialize(_serializedATN);

  size_t count = _atn.getNumberOfDecisions();
  _decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    _decisionToDFA.emplace_back(_atn.getDecisionState(i), i);
  }
}

IRParser::Initializer IRParser::_init;
