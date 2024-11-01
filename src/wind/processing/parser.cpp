#include <wind/processing/lexer.h>
#include <wind/processing/parser.h>
#include <wind/bridge/ast.h>
#include <wind/reporter/parser.h>
#include <iostream>

WindParser::WindParser(TokenStream *stream, std::string src) : stream(stream), ast(new Body({})), reporter(new ParserReport(src)) {}

bool WindParser::isKeyword(Token *src, std::string value) {
  if (src->type == Token::Type::IDENTIFIER && src->value == value) return true;
  return false;
}

bool WindParser::until(Token::Type type) {
  if (!stream->end() && stream->current()->type == type) return true;
  return false;
}

std::string WindParser::typeSignature(Token::Type until, Token::Type oruntil) {
  std::string signature = "";
  while (!this->until(until) && !this->until(oruntil)) {
    Token *token = stream->pop();
    signature += token->value;
  }
  return signature;
}

std::string WindParser::typeSignature(Token::Type while_) {
  std::string signature = "";
  while (this->until(while_)) {
    Token *token = stream->pop();
    signature += token->value;
  }
  return signature;
}

Function *WindParser::parseFn() {
  this->expect(Token::Type::IDENTIFIER, "func");
  std::string name = this->expect(Token::Type::IDENTIFIER, "function name")->value;
  this->expect(Token::Type::LPAREN, "(");
  while (!this->until(Token::Type::RPAREN)) {
    Token *arg = this->expect(Token::Type::IDENTIFIER, "argument name");
    this->expect(Token::Type::COLON, ":");
    std::string type = this->typeSignature(Token::Type::COMMA, Token::Type::RPAREN);
    if (this->until(Token::Type::COMMA)) {
      this->expect(Token::Type::COMMA, ",");
    }
  }
  this->expect(Token::Type::RPAREN, ")");
  this->expect(Token::Type::COLON, ":");
  std::string ret_type = this->typeSignature(Token::Type::IDENTIFIER);
  Body *fn_body = new Body({});
  if (this->stream->current()->type == Token::Type::LBRACE) {
    this->expect(Token::Type::LBRACE, "{");
    while (!this->until(Token::Type::RBRACE)) {
      *fn_body += std::unique_ptr<ASTNode>(
        this->Discriminate()
      );
    }
    this->expect(Token::Type::RBRACE, "}");
  }
  else {
    this->expect(Token::Type::SEMICOLON, ";");
  }
  Function *fn = new Function(name, ret_type, std::unique_ptr<Body>(fn_body));
  return nullptr;
}

ASTNode *WindParser::Discriminate() {
  if (this->isKeyword(stream->current(), "func")) {
    return this->parseFn();
  }
  else {
    Token *token = stream->current();
    this->reporter->Report(ParserReport::PARSER_ERROR, new Token(
      token->value, token->type, "Nothing (Unexpected combination)", token->range
    ), token);
    return nullptr;
  }
}

Body *WindParser::parse() {
  while (!stream->end()) {
    ASTNode *node = this->Discriminate();
    if (!node) continue;
    *this->ast += std::unique_ptr<ASTNode>(
      node
    );
  }
  return ast;
}

Token* WindParser::expect(Token::Type type, std::string str_repr) {
  Token *token = stream->pop();
  if (token->type != type) {
    this->reporter->Report(ParserReport::PARSER_ERROR, new Token(
      token->value, type, str_repr, token->range
    ), token);
  }
  return token;
}

Token* WindParser::expect(std::string value, std::string str_repr) {
  Token *token = stream->pop();
  if (token->value != value) {
    this->reporter->Report(ParserReport::PARSER_ERROR, new Token(
      value, token->type, str_repr, token->range
    ), token);
  }
  return token;
}

Token *WindParser::expect(Token::Type type, std::string value, std::string str_repr) {
  Token *token = stream->pop();
  if (token->type != type || token->value != value) {
    this->reporter->Report(ParserReport::PARSER_ERROR, new Token(
      value, type, str_repr, token->range
    ), token);
  }
  return token;
}