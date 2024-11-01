#include <string>
#include <memory>
#include <fstream>
#include <wind/processing/lexer.h>
#include <wind/processing/utils.h>
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
  return lex;
}


WindLexer::WindLexer(std::string data) : stream(data), reporter(new LexerReport()), tokens(new TokenStream()), source_back(data) {}

SymbolMatch WindLexer::MatchSymbol() {
  for (const auto &symbol : SymbolTable) {
    std::string matching(1, this->stream.current());
    for (u_int16_t i = 1; i < symbol.first.size(); i++) {
      if (this->stream.peek(i - 1) == symbol.first[i]) {
        matching += symbol.first[i];
      } else {
        break;
      }
    }
    if (matching == symbol.first) {
      return std::make_unique<std::pair<std::string, Token::Type>>(
        std::make_pair(symbol.first, symbol.second)
      );
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
  return new Token(value, Token::Type::INTEGER, "Integer", std::make_pair(start, end));
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
  return new Token(value, Token::Type::IDENTIFIER, "Identifier", std::make_pair(start, end));
}

Token *WindLexer::LexSymbol(const SymbolMatch& symbol) {
  std::string value;
  TokenPos start = this->stream.position();
  this->stream.advance(symbol->first.size());
  TokenPos end = this->stream.position();
  return new Token(value, symbol->second, symbol->first, std::make_pair(start, end));
}

Token *WindLexer::Discriminate() {
  SymbolMatch is_symbol = this->MatchSymbol();
  if ( LexUtils::digit(this->stream.current()) ) {
    return this->LexHexadecimal();
  }
  else if ( LexUtils::alphanum(this->stream.current()) ) {
    return this->LexIdentifier();
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