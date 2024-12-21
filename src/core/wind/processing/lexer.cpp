#include <string>
#include <memory>
#include <fstream>
#include <wind/processing/lexer.h>
#include <wind/processing/utils.h>
#include <wind/isc/isc.h>
#include <iostream>

CharStream::CharStream(std::string data) {
  this->buffer = std::make_unique<char[]>(data.size());
  this->size = data.size() - 1; // No NULL terminator
  std::copy(data.begin(), data.end(), this->buffer.get());
  this->pos = std::make_pair(1,1);
}

char CharStream::current() const {
  return this->buffer[this->index];
}

void CharStream::advance(u_int16_t offset) {
  if (this->index + offset >= (this->size)) {
    this->index+=offset;
    return;
  }  
  this->index+=offset;
  if (this->current() == '\n') {
    this->pos.first++;
    this->pos.second = 1;
  } else {
    this->pos.second++;
  }
}

char CharStream::peek(u_int16_t offset) const {
  if (this->index + offset >= (this->size)) {
    return '\0';
  }
  return this->buffer[this->index + offset];
}

void CharStream::reset() {
  this->index = 0;
}

bool CharStream::end() const {
  return this->index >= (this->size);
}

TokenPos CharStream::position() const {
  return this->pos;
}

TokenStream::TokenStream() {}

void TokenStream::push(Token *token) {
  this->tokens.push_back(token);
}

Token *TokenStream::pop() {
  if (this->index >= this->tokens.size()) {
    this->index++;
    return nullptr;
  }
  Token *token = this->tokens[this->index];
  this->index++;
  return token;
}

Token *TokenStream::last() const {
  if (this->tokens.size() == 0) {
    return nullptr;
  }
  return this->tokens[this->tokens.size() - 1];
}

void TokenStream::reset() {
  this->index = 0;
}

Token *TokenStream::current() const {
  if (this->index >= this->tokens.size()) {
    return nullptr;
  }
  return this->tokens[this->index];
}

void TokenStream::advance() {
  this->index++;
}

Token *TokenStream::peek(u_int16_t offset) const {
  if (this->index + offset >= this->tokens.size()) {
    return nullptr;
  }
  return this->tokens[this->index + offset];
}

bool TokenStream::end() const {
  return this->index >= this->tokens.size();
}

std::vector<Token*> TokenStream::getVec() const {
  return this->tokens;
}

void TokenStream::join(TokenStream *stream) {
  for (Token *token : stream->getVec()) {
    this->push(token);
  }
}

void TokenStream::joinAfterindex(TokenStream *stream, u_int16_t index) {
  std::vector<Token*> new_tokens;
  for (u_int16_t i = 0; i < index; i++) {
    new_tokens.push_back(this->tokens[i]);
  }
  for (Token *token : stream->getVec()) {
    new_tokens.push_back(token);
  }
  for (u_int16_t i = index; i < this->tokens.size(); i++) {
    new_tokens.push_back(this->tokens[i]);
  }
  this->tokens = new_tokens;
}


WindLexer *TokenizeFile(const char *filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Could not open file: " << filename << std::endl;
    return nullptr;
  }
  std::string data;
  std::string line;
  while (std::getline(file, line)) {
    data += line + '\n';
  }
  WindLexer *lex = new WindLexer(data);
  lex->tokenize();
  global_isc->setPath(lex->srcId, getRealPath(filename));
  global_isc->setStream(lex->srcId, lex->get());
  global_isc->newParserReport(lex->srcId, data);
  return lex;
}


WindLexer::WindLexer(std::string data) : stream(data), reporter(new LexerReport()), tokens(new TokenStream()), source_back(data) {
  srcId = global_isc->getNewSrcId();
}

SymbolMatch WindLexer::MatchSymbol() {
  for (const auto &symbol : SymbolTable) {
    std::string strsym = symbol.first;
    bool match = true;
    for (int i=0;i<strsym.size();i++) {
      if (this->stream.peek(i) == strsym[i]) {
        continue;
      }
      else {
        match=false;
        break;
      }
    }
    if (match) {
      return std::unique_ptr<std::pair<std::string, Token::Type>>(new std::pair<std::string, Token::Type>(symbol.first, symbol.second));
    }
  }
  return nullptr;
}

Token *WindLexer::LexHexadecimal() {
  std::string value;
  TokenPos start = this->stream.position();
  while ( LexUtils::hexadecimal(this->stream.current()) ) {
    value += this->stream.current();
    this->stream.advance();
  }
  TokenPos end = this->stream.position();
  end.second--;
  return new Token(value, Token::Type::INTEGER, "Integer", std::make_pair(start, end), this->srcId);
}

Token *WindLexer::LexIdentifier() {
  std::string value;
  TokenPos start = this->stream.position();
  start.second--;
  while ( LexUtils::alphanum(this->stream.current()) ) {
    value += this->stream.current();
    this->stream.advance();
  }
  TokenPos end = this->stream.position();
  end.second--;
  return new Token(value, Token::Type::IDENTIFIER, "Identifier", std::make_pair(start, end), this->srcId);
}

Token *WindLexer::LexSymbol(const SymbolMatch& symbol) {
  std::string value = symbol->first;
  TokenPos start = this->stream.position();
  start.second--;
  this->stream.advance(symbol->first.size());
  TokenPos end = this->stream.position();
  end.second += symbol->first.size()-2;
  return new Token(value, symbol->second, symbol->first, std::make_pair(start, end), this->srcId);
}

Token *WindLexer::LexString() {
  std::string value;
  TokenPos start = this->stream.position();
  start.second--;
  this->stream.advance();
  while (this->stream.current() != '"') {
    value += this->stream.current();
    this->stream.advance();
  }
  TokenPos end = this->stream.position();
  this->stream.advance();
  return new Token(value, Token::Type::STRING, "String", std::make_pair(start, end), this->srcId);
}

Token *WindLexer::LexChar() {
  std::string value;
  TokenPos start = this->stream.position();
  start.second--;
  this->stream.advance();
  while (this->stream.current() != '\'') {
    value += this->stream.current();
    this->stream.advance();
  }
  TokenPos end = this->stream.position();
  this->stream.advance();
  std::string strnum = std::to_string(value[0]);
  return new Token(strnum, Token::Type::INTEGER, "Char", std::make_pair(start, end), this->srcId);
}

Token *WindLexer::Discriminate() {
  SymbolMatch is_symbol = this->MatchSymbol();
  if ( LexUtils::digit(this->stream.current()) ) {
    return this->LexHexadecimal();
  }
  else if ( LexUtils::alphanum(this->stream.current()) ) {
    return this->LexIdentifier();
  }
  else if ( this->stream.current() == '/' && this->stream.peek() == '/' ) {
    while (this->stream.current() != '\n') {
      this->stream.advance();
    }
    return nullptr;
  }
  else if (this->stream.current() == '"') {
    return this->LexString();
  }
  else if (this->stream.current() == '\'') {
    return this->LexChar();
  }
  else if ( is_symbol != nullptr ) {
    return this->LexSymbol(is_symbol);
  }
  else if ( LexUtils::whitespace(this->stream.current()) ) {
    this->stream.advance();
    return nullptr;
  }
  else {
    this->reporter->Report(
      LexerReport::Type::LEXER_ERROR,
      "Unknown character: `" + std::string(1, this->stream.current()) + "`",
      this->stream.position()
    );
    this->stream.advance();
    return nullptr;
  }
}

TokenStream *WindLexer::tokenize() {
  while (!this->stream.end()) {
    Token *token = this->Discriminate();
    if (token != nullptr) { this->tokens->push(token); }
  }
  this->reporter->handleErrors();
  return this->tokens;
}

TokenStream *WindLexer::get() {
  return this->tokens;
}

std::string WindLexer::source() const {
  return this->source_back;
}