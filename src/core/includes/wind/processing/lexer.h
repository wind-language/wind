#include <string>
#include <memory>
#include <vector>
#ifndef LEXER_H
#define LEXER_H
#include <wind/processing/token.h>
#include <wind/reporter/lexer.h>

class CharStream {
public:
  CharStream(std::string data);
  char current() const;
  void advance(u_int16_t offset=1);
  char peek(u_int16_t offset=1) const;
  void reset();
  bool end() const;
  TokenPos position() const;

private:
  u_int16_t index=0;
  ssize_t size;
  std::unique_ptr<char[]> buffer;
  TokenPos pos;
};

class TokenStream {
public:
  TokenStream();
  void push(Token *token);
  Token *pop();
  void reset();
  Token *current() const;
  void advance();
  Token *peek(u_int16_t offset=1) const;
  bool end() const;
  Token *last() const;
  std::vector<Token*> getVec() const;
  void join(TokenStream *stream);
  void joinAfterindex(TokenStream *stream, u_int16_t index);
  const u_int16_t getIndex() const { return index; }

private:
  u_int16_t index=0;
  std::vector<Token*> tokens;
};

typedef std::unique_ptr<std::pair<std::string, Token::Type>> SymbolMatch;

class WindLexer {
public:
  WindLexer(std::string data);
  TokenStream *tokenize();
  TokenStream *get();
  std::string source() const;
  uint16_t srcId;

private:
  CharStream stream;
  LexerReport *reporter;
  TokenStream *tokens;
  std::string source_back;
  
  Token *Discriminate();
  Token *LexHexadecimal();
  Token *LexIdentifier();
  Token *LexSymbol(const SymbolMatch& type);
  Token *LexString();
  Token *LexChar();
  SymbolMatch MatchSymbol();
};

WindLexer *TokenizeFile(const char *filename);

#endif