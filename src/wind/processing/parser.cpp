#include <map>
#include <memory>
#include <wind/processing/lexer.h>
#include <wind/processing/parser.h>
#include <wind/bridge/ast.h>
#include <wind/reporter/parser.h>
#include <wind/bridge/opt_flags.h>
#include <wind/common/debug.h>

WindParser::WindParser(TokenStream *stream, std::string src) : stream(stream), ast(new Body({})), reporter(new ParserReport(src)), flag_holder(0) {}

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
  Body *fn_body = new Body({});
  std::vector<std::string> arg_types;
  this->expect(Token::Type::LPAREN, "(");
  while (!this->until(Token::Type::RPAREN)) {
    std::string arg_name = this->expect(Token::Type::IDENTIFIER, "argument name")->value;
    this->expect(Token::Type::COLON, ":");
    std::string arg_type = this->typeSignature(Token::Type::COMMA, Token::Type::RPAREN);
    arg_types.push_back(arg_type);
    *fn_body += std::unique_ptr<ASTNode>(new ArgDecl(arg_name, arg_type));
    if (this->until(Token::Type::COMMA)) {
      this->expect(Token::Type::COMMA, ",");
    }
  }
  this->expect(Token::Type::RPAREN, ")");
  this->expect(Token::Type::COLON, ":");
  std::string ret_type = this->typeSignature(Token::Type::IDENTIFIER);
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
  fn->copyArgTypes(arg_types);
  fn->flags = this->flag_holder;
  this->flag_holder = 0;
  return fn;
}

static Token::Type TOK_OP_LIST[]={
    Token::Type::PLUS,
    Token::Type::MINUS,
    Token::Type::MULTIPLY,
    Token::Type::COLON
};

bool tokIsOperator(Token *tok) {
    for (unsigned i=0;i<sizeof(TOK_OP_LIST)/sizeof(Token::Type);i++) {
        if (tok->type==TOK_OP_LIST[i]) {
            return true;
        }
    }
    return false;
}

long long fmtinttostr(std::string &str) {
  if (str.size()>2 && str[0]=='0' && str[1]=='x') {
    return std::stoll(str.substr(2), nullptr, 16);
  }
  return std::stoll(str);
}

ASTNode *WindParser::parseExprLiteral() {
  switch (this->stream->current()->type) {
    case Token::INTEGER:
      return new Literal(
        fmtinttostr(this->stream->pop()->value)
      );
    default:
      return nullptr;
  }
}

ASTNode *WindParser::parseExprFnCall() {
  std::string name = this->expect(Token::Type::IDENTIFIER, "function name")->value;
  this->expect(Token::Type::LPAREN, "(");
  std::vector<std::unique_ptr<ASTNode>> args;
  while (!this->until(Token::Type::RPAREN)) {
    ASTNode *arg = this->parseExpr(0);
    args.push_back(std::unique_ptr<ASTNode>(arg));
    if (this->until(Token::Type::COMMA)) {
      this->expect(Token::Type::COMMA, ",");
    }
  }
  this->expect(Token::Type::RPAREN, ")");
  return new FnCall(
    name, std::move(args)
  );
}

ASTNode *WindParser::parseExprPrimary() {
  switch (this->stream->current()->type) {
    // case Token::STRING
    case Token::INTEGER:
      return this->parseExprLiteral();

    case Token::IDENTIFIER: {
      if (this->stream->peek()->type == Token::LPAREN) {
        return this->parseExprFnCall();
      } else {
        return new VariableRef(
          this->stream->pop()->value
        );
      }
    }

    default:
      Token *token = this->stream->pop();
      this->reporter->Report(ParserReport::PARSER_ERROR, new Token(
      token->value, token->type, "Nothing (Unexpected token in expression)", token->range
    ), token);
      return nullptr;
  }
}

ASTNode *WindParser::parseExpr(int precedence) {
  ASTNode *left = this->parseExprPrimary();
  if (!left) {
    return nullptr;
  }
  return this->parseExprBinOp(left, precedence);
}

ASTNode *WindParser::parseExprSemi() {
  ASTNode *root = this->parseExpr(0);
  if (!root) {
    return nullptr;
  }
  this->expect(Token::Type::SEMICOLON, ";");
  return root;
}

int getOpPrecedence(Token *tok) {
  switch (tok->type) {
    case Token::Type::PLUS:
    case Token::Type::MINUS:
      return 1;
    case Token::Type::MULTIPLY:
    case Token::Type::DIVIDE:
      return 2;
    // TODO: More
    default:
      return 0;
  }
}

ASTNode *WindParser::parseExprBinOp(ASTNode *left, int precedence) {
  while (this->stream->current() && tokIsOperator(this->stream->current())) {
    Token *op = this->stream->pop();
    int new_precedence = getOpPrecedence(op);

    if (new_precedence < precedence) {
      break;
    }

    ASTNode *right = this->parseExprPrimary();

    if (this->stream->current() && tokIsOperator(this->stream->current())) {
      int next_precedence = getOpPrecedence(this->stream->current());
      if (new_precedence < next_precedence) {
        right = this->parseExprBinOp(right, new_precedence+1);
      }
    }

    left = new BinaryExpr(
      std::unique_ptr<ASTNode>(left),
      std::unique_ptr<ASTNode>(right),
      op->value
    );
  }
  return left;
}

Return *WindParser::parseRet() {
  this->expect(Token::Type::IDENTIFIER, "return");
  ASTNode *ret_expr=nullptr;
  if (stream->current()->type != Token::Type::SEMICOLON) {
    ret_expr = this->parseExprSemi();
  }
  return new Return(std::unique_ptr<ASTNode>(ret_expr));
}

LocalDecl *WindParser::parseVarDecl() {
  this->expect(Token::Type::IDENTIFIER, "var");
  std::string name = this->expect(Token::Type::IDENTIFIER, "variable name")->value;
  this->expect(Token::Type::COLON, ":");
  std::string type = this->typeSignature(Token::Type::IDENTIFIER);
  if (this->stream->current()->type == Token::Type::ASSIGN) {
    this->expect(Token::Type::ASSIGN, "=");
    ASTNode *expr = this->parseExprSemi();
    return new LocalDecl(name, type, std::unique_ptr<ASTNode>(expr));
  }
  else {
    this->expect(Token::Type::SEMICOLON, ";");
    return new LocalDecl(name, type, nullptr);
  }
}

static std::map<std::string, FnFlags> FLAGS_MAP = {
  {"stack", PURE_STACK},
  {"abi", PURE_ABI},
  {"noabi", PURE_NOABI},
  {"expr", PURE_EXPR},
  {"logue", PURE_LOGUE}
};

FnFlags macroIntoFlag(std::string name) {
  if (FLAGS_MAP.find(name) != FLAGS_MAP.end()) {
    return FLAGS_MAP[name];
  }
  return 0;
}

void WindParser::parseMacro() {
  this->expect(Token::Type::AT, "@");
  std::string name = this->expect(Token::Type::IDENTIFIER, "macro name")->value;

  if (name == "pure") {
    this->expect(Token::Type::LBRACKET, "[");
    while (!this->until(Token::Type::RBRACKET)) {
      Token *flag = this->expect(Token::Type::IDENTIFIER, "flag");
      FnFlags flagged= macroIntoFlag(flag->value);
      if (flagged) {
        this->flag_holder |= flagged;
      } else {
        this->reporter->Report(ParserReport::PARSER_ERROR, new Token(
          flag->value, flag->type, "Nothing (Unexpected flag)", flag->range
        ), flag);
      }
    }
    this->expect(Token::Type::RBRACKET, "]");
  }
  else {
    Token *token = stream->pop();
    this->reporter->Report(ParserReport::PARSER_ERROR, new Token(
      token->value, token->type, "Nothing (Unexpected macro)", token->range
    ), token);
  }
}

InlineAsm *WindParser::parseInlAsm() {
  this->expect(Token::Type::IDENTIFIER, "asm");
  this->expect (Token::Type::LBRACE, "{");
  std::string code="";
  while (!this->until(Token::Type::RBRACE)) {
    Token *token = stream->pop();
    if (token->type == Token::Type::SEMICOLON) {
      code += "\n";
    }
    else {
      code += token->value;
      if (this->stream->current()->type != Token::Type::COMMA && token->type != Token::Type::QMARK) {
        code += " ";
      }
    }
  }
  this->expect(Token::Type::RBRACE, "}");
  return new InlineAsm(code);
}

ASTNode *WindParser::Discriminate() {
  if (this->isKeyword(stream->current(), "func")) {
    return this->parseFn();
  }
  else if (this->isKeyword(stream->current(), "return")) {
    return this->parseRet();
  }
  else if (this->isKeyword(stream->current(), "var")) {
    return this->parseVarDecl();
  }
  else if (this->isKeyword(stream->current(), "asm")) {
    return this->parseInlAsm();
  }
  else if (this->stream->current()->type == Token::Type::AT) {
    this->parseMacro();
  }
  else {
    Token *token = stream->current();
    this->reporter->Report(ParserReport::PARSER_ERROR, new Token(
      token->value, token->type, "Nothing (Unexpected combination)", token->range
    ), token);
  }
  return nullptr;
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
