#include <wind/processing/lexer.h>
#include <wind/bridge/ast.h>
#include <wind/reporter/parser.h>
#ifndef PARSER_H
#define PARSER_H

class WindParser {
public:
  WindParser(TokenStream *stream, std::string src_path);
  ASTNode *Discriminate();
  Body *parse();

private:
  Token *expect(Token::Type type, std::string str_repr);
  Token *expect(std::string value, std::string str_repr);
  Token *expect(Token::Type type, std::string value, std::string str_repr);

  bool isKeyword(Token *src, std::string value);
  bool until(Token::Type type);
  std::string typeSignature(Token::Type until, Token::Type oruntil);
  std::string typeSignature(Token::Type while_);
  Function *parseFn();
  Return *parseRet();
  LocalDecl *parseVarDecl();
  InlineAsm *parseInlAsm();

  ASTNode *parseMacro();

  ASTNode *parseExprBinOp(ASTNode *left, int precedence);
  ASTNode *parseExprFnCall();
  ASTNode *parseExprLiteral();
  ASTNode *parseExprPrimary();
  ASTNode *parseExpr(int precedence);
  ASTNode *parseExprSemi();

  void pathWork(std::string relative, Token *token_ref);

private:
  TokenStream *stream;
  std::string file_path;
  Body *ast;
  int flag_holder=0;
};

#endif