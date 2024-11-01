#include <string>
#include <vector>
#include <wind/processing/lexer.h>
#ifndef LEXER_REP_H
#define LEXER_REP_H

class LexerReport {
public:
  enum Type {
    LEXER_ERROR,
    LEXER_WARNING
  };

  LexerReport() {}
  void Report(
    LexerReport::Type type,
    std::string message,
    TokenPos position
  );
  void handleErrors();

private:
  bool is_exiting = false;

  std::vector< // [type, [message, position]]
    std::pair<
      LexerReport::Type,
      std::pair<
        std::string,
        TokenPos
      >
    >
  > error_cache;
};

#endif