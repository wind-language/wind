#include <wind/reporter/lexer.h>
#include <string>
#include <vector>
#include <iostream>

void LexerReport::Report(LexerReport::Type type, std::string message, TokenPos position) {
  this->error_cache.push_back(
    std::make_pair(type, std::make_pair(message, position))
  );
  if (!this->is_exiting && type == LexerReport::Type::LEXER_ERROR) {
    this->is_exiting = true;
  }
}

void LexerReport::handleErrors() {
  if (this->error_cache.size() == 0) {
    return;
  }
  for (auto error : this->error_cache) {
    if (error.first == LexerReport::Type::LEXER_WARNING) {
      std::cerr << "\x1b[33m[WARNING] \x1b[0m\x1b[1m" << error.second.first << "\x1b[0m\x1b[3m at \x1b[0m\x1b[36m(" << error.second.second.first << ":" << error.second.second.second << ")\x1b[0m" << std::endl;
    }
    else if (error.first == LexerReport::Type::LEXER_ERROR) {
      std::cerr << "\x1b[31m[ERROR] \x1b[0m\x1b[1m" << error.second.first << "\x1b[0m\x1b[3m at \x1b[0m\x1b[36m(" << error.second.second.first << ":" << error.second.second.second << ")\x1b[0m" << std::endl;
    }
  }
  if (this->is_exiting) {
    exit(1);
  }
}