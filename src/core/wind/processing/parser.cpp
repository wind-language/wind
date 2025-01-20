#include <map>
#include <memory>
#include <wind/processing/lexer.h>
#include <wind/processing/parser.h>
#include <wind/processing/utils.h>
#include <wind/bridge/ast.h>
#include <wind/reporter/parser.h>
#include <wind/bridge/flags.h>
#include <wind/common/debug.h>
#include <wind/isc/isc.h>
#include <filesystem>

#ifndef WIND_STD_PATH
#define WIND_STD_PATH ""
#endif

#ifndef WIND_PKGS_PATH
#define WIND_PKGS_PATH ""
#endif

ParserReport *GetReporter(Token *src) {
  return global_isc->getParserReport(src->srcId);
}

WindParser::WindParser(TokenStream *stream, std::string src_path) :
  stream(stream),
  ast(new Body({})),
  flag_holder(0),
  file_path(src_path) {}

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
  bool first = true;
  while (!this->until(until) && !this->until(oruntil)) {
    Token *token = stream->pop();
    if (isKeyword(token, "ptr") && this->stream->current()->type == Token::Type::LESS) {
      signature += token->value;
      signature += this->expect(Token::Type::LESS, "<")->value;
      signature += this->typeSignature(Token::Type::IDENTIFIER);
      signature += this->expect(Token::Type::GREATER, ">")->value;
    } else {
      if (first){
        first=false;
      } else {
        signature += " ";
      }
      signature += token->value;
    }
  }
  return signature;
}

std::string WindParser::typeSignature(Token::Type while_) {
  std::string signature = "";

  if (this->stream->current()->type == Token::Type::LBRACKET) {
    signature += this->expect(Token::Type::LBRACKET, "[")->value;
    signature += this->typeSignature(Token::Type::IDENTIFIER);
    if (this->stream->current()->type == Token::Type::SEMICOLON) {
      signature += this->expect(Token::Type::SEMICOLON, ";")->value;
      signature += this->expect(Token::Type::INTEGER, "array size")->value;
    }
    signature += this->expect(Token::Type::RBRACKET, "]")->value;
    return signature;
  }
  else if (isKeyword(this->stream->current(), "ptr") && this->stream->peek()->type == Token::Type::LESS) {
    signature += this->expect(Token::Type::IDENTIFIER, "ptr")->value;
    signature += this->expect(Token::Type::LESS, "<")->value;
    signature += this->typeSignature(Token::Type::IDENTIFIER);
    signature += this->expect(Token::Type::GREATER, ">")->value;
    return signature;
  }

  bool first = true;
  while (this->until(while_)) {
    Token *token = stream->pop();
    if (first) {
      first = false;
    } else {
      signature += " ";
    }
    signature += token->value;
  }
  return signature;
}


Function *WindParser::parseFn() {
  Token *fn_ref_tok = this->expect(Token::Type::IDENTIFIER, "func");
  std::string name = this->expect(Token::Type::IDENTIFIER, "function name")->value;
  Body *fn_body = new Body({});
  std::vector<std::string> arg_types;
  this->expect(Token::Type::LPAREN, "(");
  while (!this->until(Token::Type::RPAREN)) {
    if (this->stream->current()->type == Token::Type::VARDC) {
      this->expect(Token::Type::VARDC, "...");
      this->flag_holder |= FN_VARIADIC;
      break;
    }
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
  bool isDefined = this->stream->current()->type == Token::Type::LBRACE;
  if (this->stream->current()->type == Token::Type::LBRACE) {
    this->expect(Token::Type::LBRACE, "{");
    while (!this->until(Token::Type::RBRACE)) {
      *fn_body += std::unique_ptr<ASTNode>(
        this->DiscriminateBody()
      );
    }
    this->expect(Token::Type::RBRACE, "}");
  }
  else {
    this->expect(Token::Type::SEMICOLON, ";");
  }
  Function *fn = new Function(name, ret_type, std::unique_ptr<Body>(fn_body));
  fn->isDefined = isDefined;
  fn->metadata = std::filesystem::path(global_isc->getPath(fn_ref_tok->srcId)).filename().string();
  fn->copyArgTypes(arg_types);
  fn->flags = this->flag_holder;
  this->flag_holder = 0;
  return fn;
}

static Token::Type TOK_OP_LIST[]={
  Token::Type::PLUS,
  Token::Type::MINUS,
  Token::Type::MULTIPLY,
  Token::Type::DIVIDE,
  Token::Type::MODULO,
  Token::Type::ASSIGN,
  Token::Type::PLUS_ASSIGN,
  Token::Type::MINUS_ASSIGN,
  Token::Type::EQ,
  Token::Type::LESS,
  Token::Type::GREATER,
  Token::Type::LESSEQ,
  Token::Type::LOGAND,
  Token::Type::INCREMENT,
  Token::Type::DECREMENT,
  Token::Type::LOGOR,
  Token::Type::GREATEREQ,
  Token::Type::NOTEQ,
  Token::Type::OR,
  Token::Type::AND,
  Token::Type::XOR,
  Token::Type::CAST_SYMBOL
};

bool tokIsOperator(Token *tok) {
    for (unsigned i=0;i<sizeof(TOK_OP_LIST)/sizeof(Token::Type);i++) {
        if (tok->type==TOK_OP_LIST[i]) {
            return true;
        }
    }
    return false;
}

ASTNode *WindParser::parseExprLiteral(bool negative) {
  switch (this->stream->current()->type) {
    case Token::INTEGER:
      return new Literal(
        negative ? -fmtinttostr(this->stream->pop()->value) : fmtinttostr(this->stream->pop()->value)
      );
    case Token::STRING:
      return new StringLiteral(
        this->stream->pop()->value
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
    case Token::STRING:
    case Token::INTEGER:
      return this->parseExprLiteral();

    case Token::IDENTIFIER: {
      if (isKeyword(this->stream->current(), "true")) {
        this->stream->pop();
        return new Literal(1);
      } else if (isKeyword(this->stream->current(), "false") || isKeyword(this->stream->current(), "Null")) {
        this->stream->pop();
        return new Literal(0);
      }
      else if (isKeyword(this->stream->current(), "guard") && this->stream->peek()->type == Token::Type::NOT) {
        // ptr guard
        this->expect(Token::Type::IDENTIFIER, "guard");
        this->expect(Token::Type::NOT, "!");
        this->expect(Token::Type::LBRACKET, "[");
        ASTNode *value = this->parseExpr(0);
        this->expect(Token::Type::RBRACKET, "]");
        return new PtrGuard(
          std::unique_ptr<ASTNode>(value)
        );
      }
      else if (isKeyword(this->stream->current(), "sizeof") && this->stream->peek()->type == Token::Type::LESS) {
        // sizeof
        this->expect(Token::Type::IDENTIFIER, "sizeof");
        this->expect(Token::Type::LESS, "<");
        std::string type = this->typeSignature(Token::Type::GREATER, Token::Type::GREATER);
        this->expect(Token::Type::GREATER, ">");
        return new SizeOf(
          type
        );
      }
      else if (
        this->ast->consts_table.find(this->stream->current()->value) != this->ast->consts_table.end()
      ) {
        return this->ast->consts_table[this->stream->pop()->value];
      }

      if (this->stream->peek()->type == Token::LPAREN) {
        return this->parseExprFnCall();
      } else if (this->stream->peek()->type == Token::LBRACKET) {
        std::string name = this->stream->pop()->value;
        this->expect(Token::Type::LBRACKET, "[");
        ASTNode *index = this->parseExpr(0);
        this->expect(Token::Type::RBRACKET, "]");
        return new VarAddressing(
          name, index
        );
      }
      else {
        return new VariableRef(
          this->stream->pop()->value
        );
      }
    }

    case Token::AND : {
      this->expect(Token::Type::AND, "&");
      return new VarAddressing(
        this->expect(Token::Type::IDENTIFIER, "variable name")->value
      );
    }

    case Token::LPAREN : {
      this->expect(Token::Type::LPAREN, "(");
      ASTNode *expr = this->parseExpr(0);
      this->expect(Token::Type::RPAREN, ")");
      return expr;
    }
    case Token::MINUS : {
      // a negative number
      this->expect(Token::Type::MINUS, "-");
      if (this->stream->current()->type == Token::INTEGER) {
        return this->parseExprLiteral(true);
      } else {
        return new BinaryExpr(
          std::unique_ptr<ASTNode>(new Literal(0)),
          std::unique_ptr<ASTNode>(this->parseExprPrimary()),
          "-"
        );
      }
    }
  
    default:
      Token *token = this->stream->pop();
      GetReporter(token)->Report(ParserReport::PARSER_ERROR, new Token(
      token->value, token->type, "Nothing (Unexpected token in expression)", token->range, token->srcId), token);
      return nullptr;
  }
}

ASTNode *WindParser::parseExpr(int precedence) {
  ASTNode *left = this->parseExprPrimary();
  if (!left) {
    return nullptr;
  }
  ASTNode *enode = left;
  enode = this->parseExprBinOp(enode, precedence);

  while (this->stream->current()->type == Token::Type::LBRACKET) {
    this->expect(Token::Type::LBRACKET, "[");
    ASTNode *index = this->parseExpr(0);
    this->expect(Token::Type::RBRACKET, "]");
    enode = new GenericIndexing(
      std::unique_ptr<ASTNode>(index),
      std::unique_ptr<ASTNode>(enode)
    );
    enode = this->parseExprBinOp(enode, precedence);
  }

  return enode;
}

ASTNode *WindParser::parseExprSemi() {
  ASTNode *root = this->parseExpr(0);
  if (!root) {
    return nullptr;
  }
  this->expect(Token::Type::SEMICOLON, ";");
  return root;
}

ASTNode *WindParser::parseExprColon() {
  ASTNode *root = this->parseExpr(0);
  if (!root) {
    return nullptr;
  }
  this->expect(Token::Type::COLON, ":");
  return root;
}

int getOpPrecedence(Token *tok) {
  switch (tok->type) {
    case Token::Type::PLUS:
    case Token::Type::MINUS:
      return 1;
    case Token::Type::MULTIPLY:
    case Token::Type::DIVIDE:
    case Token::Type::MODULO:
      return 2;
    
    case Token::Type::EQ:
    case Token::Type::LESS:
    case Token::Type::GREATER:
    case Token::Type::LESSEQ:
      return 2;
    case Token::Type::CAST_SYMBOL:
      return 3;

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

    ASTNode *right;
    if (op->type == Token::Type::INCREMENT) {
      right = new Literal(1);
      left = new BinaryExpr(
        std::unique_ptr<ASTNode>(left),
        std::unique_ptr<ASTNode>(right),
        "+="
      );
      continue;
    }
    else if (op->type == Token::Type::DECREMENT) {
      right = new Literal(1);
      left = new BinaryExpr(
        std::unique_ptr<ASTNode>(left),
        std::unique_ptr<ASTNode>(right),
        "-="
      );
      continue;
    }
    else if (op->type == Token::Type::CAST_SYMBOL) {
      left = new TypeCast(
        this->typeSignature(Token::Type::IDENTIFIER),
        std::unique_ptr<ASTNode>(left)
      );
      continue;
    }
    right = this->parseExprPrimary();

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
  } else {
    this->expect(Token::Type::SEMICOLON, ";");
  }
  return new Return(std::unique_ptr<ASTNode>(ret_expr));
}

VariableDecl *WindParser::parseVarDecl() {
  this->expect(Token::Type::IDENTIFIER, "var");
  std::vector<std::string> names;
  if (this->stream->current()->type == Token::Type::LBRACKET) {
    // Multi declaration
    this->expect(Token::Type::LBRACKET, "[");
    while (!this->until(Token::Type::RBRACKET)) {
      names.push_back(this->expect(Token::Type::IDENTIFIER, "variable name")->value);
      if (this->until(Token::Type::COMMA)) {
        this->expect(Token::Type::COMMA, ",");
      }
    }
    this->expect(Token::Type::RBRACKET, "]");
  } else {
    names.push_back(this->expect(Token::Type::IDENTIFIER, "variable name")->value);
  }
  this->expect(Token::Type::COLON, ":");
  std::string type = this->typeSignature(Token::Type::IDENTIFIER);
  ASTNode *expr = nullptr;
  if (this->stream->current()->type == Token::Type::ASSIGN) {
    this->expect(Token::Type::ASSIGN, "=");
    expr = this->parseExprSemi();
  }
  else {
    this->expect(Token::Type::SEMICOLON, ";");
  }
  return new VariableDecl(names, type, std::unique_ptr<ASTNode>(expr));
}

static std::map<std::string, FnFlags> FLAGS_MAP = {
  {"stack", PURE_STACK},
  {"abi", PURE_ABI},
  {"noabi", PURE_NOABI},
  {"expr", PURE_EXPR},
  {"logue", PURE_LOGUE},
  {"stchk", PURE_STCHK}
};

FnFlags macroIntoFlag(std::string name) {
  if (FLAGS_MAP.find(name) != FLAGS_MAP.end()) {
    return FLAGS_MAP[name];
  }
  return 0;
}

void WindParser::pathWorkInclude(std::string relative, Token *token_ref) {
  std::string path;
  if (relative[0] != '#') {
    std::string this_path = global_isc->getPath(token_ref->srcId);
    std::string folder = std::filesystem::path(this_path).parent_path().string();
    path = std::filesystem::path(folder).append(relative).string();
  } else {
    path = std::filesystem::path(WIND_STD_PATH).append(relative.substr(1)).string();
    if (path[0] != '/') {
      path = std::filesystem::path(getExeDir()).append(path).string();
    }
  }
  int srcId = global_isc->getSrcId(path);
  if (srcId == -1) {
    if (global_isc->workOnInclude(path)) {
      GetReporter(token_ref)->Report(ParserReport::PARSER_ERROR, new Token(
        token_ref->value, token_ref->type, "Nothing Failed to include: " + path, token_ref->range, token_ref->srcId
      ), token_ref);
    }
    Body *commit_diff = global_isc->commitAST(this->ast);
    if (commit_diff != nullptr) {
      this->ast = commit_diff;
    }
  }
  else {
    /* GetReporter(token_ref)->Report(ParserReport::PARSER_WARNING, new Token(
      token_ref->value, token_ref->type, "Nothing (Already included)", token_ref->range, token_ref->srcId
    ), token_ref); */
  }
}

void WindParser::pathWorkImport(std::string relative, Token *token_ref) {
  std::string path;
  if (relative[0] != '#') {
    std::string this_path = global_isc->getPath(token_ref->srcId);
    std::string folder = std::filesystem::path(this_path).parent_path().string();
    path = std::filesystem::path(folder).append(relative).string();
  } else {
    path = std::filesystem::path(WIND_PKGS_PATH).append(relative.substr(1)).string();
    if (path[0] != '/') {
      path = std::filesystem::path(getExeDir()).append(path).string();
    }
  }
  int srcId = global_isc->getSrcId(path);
  if (srcId == -1) {
    if (global_isc->workOnImport(path)) {
      GetReporter(token_ref)->Report(ParserReport::PARSER_ERROR, new Token(
        token_ref->value, token_ref->type, "Nothing, Failed to import: " + path , token_ref->range, token_ref->srcId
      ), token_ref);
    }
    Body *commit_diff = global_isc->commitAST(this->ast);
    if (commit_diff != nullptr) {
      this->ast = commit_diff;
    }
  }
  else {
    /* GetReporter(token_ref)->Report(ParserReport::PARSER_WARNING, new Token(
      token_ref->value, token_ref->type, "Nothing (Already included)", token_ref->range, token_ref->srcId
    ), token_ref); */
  }
}

ASTNode* WindParser::parseMacro() {
  this->expect(Token::Type::AT, "@");
  Token *macro_tok = this->expect(Token::Type::IDENTIFIER, "macro name");
  std::string name = macro_tok->value;

  if (name == "pure") {
    this->expect(Token::Type::LBRACKET, "[");
    while (!this->until(Token::Type::RBRACKET)) {
      Token *flag = this->expect(Token::Type::IDENTIFIER, "flag");
      FnFlags flagged= macroIntoFlag(flag->value);
      if (flagged) {
        this->flag_holder |= flagged;
      } else {
        GetReporter(flag)->Report(ParserReport::PARSER_ERROR, new Token(
          flag->value, flag->type, "Nothing (Unexpected flag)", flag->range, flag->srcId), flag);
      }
    }
    this->expect(Token::Type::RBRACKET, "]");
  }
  else if (name == "extern") {
    this->flag_holder |= FN_EXTERN;
  }
  else if (name == "pub") {
    this->flag_holder |= FN_PUBLIC;
  }
  else if (name == "include") {
    if (this->stream->current()->type != Token::Type::LBRACKET) {
      Token *path = this->expect(Token::Type::STRING, "include path");
      this->pathWorkInclude(path->value, macro_tok);
    } else {
      this->expect(Token::Type::LBRACKET, "[");
      while (!this->until(Token::Type::RBRACKET)) {
        Token *path = this->expect(Token::Type::STRING, "include path");
        this->pathWorkInclude(path->value, path);
      }
      this->expect(Token::Type::RBRACKET, "]");
    }
  }
  else if (name == "import") {
    if (this->stream->current()->type != Token::Type::LBRACKET) {
      Token *path = this->expect(Token::Type::STRING, "import path");
      this->pathWorkImport(path->value, macro_tok);
    } else {
      this->expect(Token::Type::LBRACKET, "[");
      while (!this->until(Token::Type::RBRACKET)) {
        Token *path = this->expect(Token::Type::STRING, "import path");
        this->pathWorkImport(path->value, path);
      }
      this->expect(Token::Type::RBRACKET, "]");
    }
  }
  else if (name == "linkflag") {
    this->expect(Token::Type::LPAREN, "(");
    while (!this->until(Token::Type::RPAREN)) {
      Token *flag = this->expect(Token::Type::STRING, "link flag");
      global_isc->addLdFlag(flag->value);
    }
    this->expect(Token::Type::RPAREN, ")");
  }
  else if (name == "type") {
    std::string type = this->typeSignature(Token::Type::IDENTIFIER);
    this->expect(Token::Type::ASSIGN, "=");
    std::string value = this->typeSignature(Token::Type::IDENTIFIER);
    this->expect(Token::Type::SEMICOLON, ";");
    return new TypeDecl(type, value);
  }
  else if (name == "const") {
    std::string c_name = this->expect(Token::Type::IDENTIFIER, "const name")->value;
    this->expect(Token::Type::ASSIGN, "=");
    ASTNode *expr = this->parseExprSemi();
    this->ast->consts_table[c_name] = expr;
  }
  else {
    Token *token = stream->pop();
    GetReporter(token)->Report(ParserReport::PARSER_ERROR, new Token(
      token->value, token->type, "Nothing (Unexpected macro)", token->range, token->srcId
    ), token);
  }
  return nullptr;
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
  code.pop_back();
  return new InlineAsm(code);
}

Body *WindParser::parseBranchBody() {
  Body *branch_body = new Body({});
  if (this->stream->current()->type == Token::Type::LBRACE) {
    this->expect(Token::Type::LBRACE, "{");
    while (!this->until(Token::Type::RBRACE)) {
      *branch_body += std::unique_ptr<ASTNode>(
        this->DiscriminateBody()
      );
    }
    this->expect(Token::Type::RBRACE, "}");
  }
  else {
    *branch_body += std::unique_ptr<ASTNode>(
      this->DiscriminateBody()
    );
  }
  return branch_body;
}

Branching *WindParser::parseBranch() {
  this->expect(Token::Type::IDENTIFIER, "branch");
  Branching *branch = new Branching();
  this->expect(Token::Type::LBRACKET, "[");
  while (!this->until(Token::Type::RBRACKET)) {
    // else
    if (this->isKeyword(stream->current(), "else")) {
      this->expect(Token::Type::IDENTIFIER, "else");
      this->expect(Token::Type::COLON, ":");
      branch->setElseBranch(this->parseBranchBody());
      break;
    }
    
    
    // condition
    else {
      ASTNode *condition = this->parseExprColon();
      branch->addBranch(
        std::unique_ptr<ASTNode>(condition),
        std::unique_ptr<Body>(this->parseBranchBody())
      );
    }
  }
  this->expect(Token::Type::RBRACKET, "]");
  return branch;
}

Looping *WindParser::parseLoop() {
  this->expect(Token::Type::IDENTIFIER, "loop");
  this->expect(Token::Type::LBRACKET, "[");
  Looping *loop = new Looping();
  loop->setCondition(this->parseExpr(0));
  this->expect(Token::Type::RBRACKET, "]");
  loop->setBody( this->parseBranchBody() );
  return loop;
}


GlobalDecl *WindParser::parseGlobDecl() {
  this->expect(Token::Type::IDENTIFIER, "global");
  std::string name = this->expect(Token::Type::IDENTIFIER, "variable name")->value;
  this->expect(Token::Type::COLON, ":");
  std::string type = this->typeSignature(Token::Type::IDENTIFIER);
  if (this->stream->current()->type == Token::Type::ASSIGN) {
    this->expect(Token::Type::ASSIGN, "=");
    ASTNode *expr = this->parseExprSemi();
    return new GlobalDecl(name, type, std::unique_ptr<ASTNode>(expr));
  }
  else {
    this->expect(Token::Type::SEMICOLON, ";");
    return new GlobalDecl(name, type, nullptr);
  }
}

TryCatch *WindParser::parseTryCatch() {
  TryCatch *try_catch = new TryCatch();
  this->expect(Token::Type::IDENTIFIER, "try");
  this->expect(Token::Type::LBRACE, "{");
  Body *try_body = new Body({});
  while (!this->until(Token::Type::RBRACE)) {
    *try_body += std::unique_ptr<ASTNode>(
      this->DiscriminateBody()
    );
  }
  try_catch->setTryBody(try_body);
  this->expect(Token::Type::RBRACE, "}");
  this->expect(Token::Type::LBRACKET, "[");
  this->stream->advance(-1);
  while (this->stream->current()->type == Token::Type::LBRACKET) {
    this->expect(Token::Type::LBRACKET, "[");
    std::string block_name = this->expect(Token::Type::IDENTIFIER, "catch block name")->value;
    this->expect(Token::Type::RBRACKET, "]");
    this->expect(Token::Type::ARROW, "->");
    this->expect(Token::Type::LBRACE, "{");
    Body *catch_body = new Body({});
    while (!this->until(Token::Type::RBRACE)) {
      *catch_body += std::unique_ptr<ASTNode>(
        this->DiscriminateBody()
      );
    }
    this->expect(Token::Type::RBRACE, "}");
    try_catch->addCatchBlock(block_name, catch_body);
  }
  if (this->isKeyword(stream->current(), "finally")) {
    this->expect(Token::Type::IDENTIFIER, "finally");
    this->expect(Token::Type::LBRACE, "{");
    Body *finally_body = new Body({});
    while (!this->until(Token::Type::RBRACE)) {
      *finally_body += std::unique_ptr<ASTNode>(
        this->DiscriminateBody()
      );
    }
    this->expect(Token::Type::RBRACE, "}");
    try_catch->setFinallyBlock(finally_body);
  }
  return try_catch;
}

ASTNode *WindParser::DiscriminateTop() {
  if (this->isKeyword(stream->current(), "func")) {
    return this->parseFn();
  }
  else if (this->isKeyword(stream->current(), "global")) {
    return this->parseGlobDecl();
  }
  else if (this->stream->current()->type == Token::Type::AT) {
    return this->parseMacro();
  }
  else {
    Token *token = stream->current();
    GetReporter(token)->Report(ParserReport::PARSER_ERROR, new Token(
      token->value, token->type, "Nothing (Unexpected combination)", token->range, token->srcId
    ), token);
  }
  return nullptr;
}

ASTNode *WindParser::DiscriminateBody() {
  if (this->isKeyword(stream->current(), "return")) {
    return this->parseRet();
  }
  else if (this->isKeyword(stream->current(), "var")) {
    return this->parseVarDecl();
  }
  else if (this->isKeyword(stream->current(), "asm")) {
    return this->parseInlAsm();
  } 
  else if (this->isKeyword(stream->current(), "branch")) {
    return this->parseBranch();
  }
  else if (this->isKeyword(stream->current(), "loop")) {
    return this->parseLoop();
  }
  else if (this->isKeyword(stream->current(), "break")) {
    this->expect(Token::Type::IDENTIFIER, "break");
    this->expect(Token::Type::SEMICOLON, ";");
    return new Break();
  }
  else if (this->isKeyword(stream->current(), "continue")) {
    this->expect(Token::Type::IDENTIFIER, "continue");
    this->expect(Token::Type::SEMICOLON, ";");
    return new Continue();
  }
  else if (this->isKeyword(stream->current(), "try")) {
    return this->parseTryCatch();
  }
  else {
    return this->parseExprSemi();
  }
}

Body *WindParser::parse() {
  while (!stream->end()) {
    ASTNode *node = this->DiscriminateTop();
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
    GetReporter(token)->Report(ParserReport::PARSER_ERROR, new Token(
      token->value, type, str_repr, token->range, token->srcId
    ), token);
  }
  return token;
}

Token* WindParser::expect(std::string value, std::string str_repr) {
  Token *token = stream->pop();
  if (token->value != value) {
    GetReporter(token)->Report(ParserReport::PARSER_ERROR, new Token(
      value, token->type, str_repr, token->range, token->srcId
    ), token);
  }
  return token;
}

Token *WindParser::expect(Token::Type type, std::string value, std::string str_repr) {
  Token *token = stream->pop();
  if (token->type != type || token->value != value) {
    GetReporter(token)->Report(ParserReport::PARSER_ERROR, new Token(
      value, type, str_repr, token->range, token->srcId
    ), token);
  }
  return token;
}
