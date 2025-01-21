#include <string>
#include <map>
#include <stdint.h>

#ifndef TOKEN_H
#define TOKEN_H

typedef std::pair<u_int16_t, u_int16_t> TokenPos;
typedef std::pair<TokenPos, TokenPos> TokenRange;

typedef uint16_t TokenSrcId;

class Token {
public:
  enum Type {
    IDENTIFIER,
    INTEGER,
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    MODULO,
    ASSIGN,
    LPAREN,
    RPAREN,
    COLON,
    LBRACE,
    RBRACE,
    COMMA,
    SEMICOLON,
    LBRACKET,
    RBRACKET,
    AT,
    QMARK,
    STRING,
    VARDC,
    AND,
    OR,
    XOR,
    LOGAND, // &&
    LOGOR, // ||
    EQ,
    LESS,
    GREATER,
    LESSEQ,
    GREATEREQ,
    PLUS_ASSIGN,
    MINUS_ASSIGN,
    INCREMENT,
    DECREMENT,
    NOTEQ,
    NOT,
    ARROW,
    CAST_SYMBOL
  };
  
  std::string value;
  Token::Type type;
  std::string name;
  TokenRange range;
  TokenSrcId srcId;

  Token(
    std::string value,
    Token::Type type,
    std::string name,
    TokenRange range,
    TokenSrcId srcId
  ) : value(value), type(type), name(name), range(range), srcId(srcId) {}
};

std::vector<std::pair<std::string, Token::Type>> const SymbolTable = {
  // double char symbols
  {"==", Token::Type::EQ},
  {"&&", Token::Type::LOGAND},
  {"||", Token::Type::LOGOR},
  {"<=", Token::Type::LESSEQ},
  {">=", Token::Type::GREATEREQ},
  {"+=", Token::Type::PLUS_ASSIGN},
  {"-=", Token::Type::MINUS_ASSIGN},
  {"++", Token::Type::INCREMENT},
  {"--", Token::Type::DECREMENT},
  {"!=", Token::Type::NOTEQ},
  {"->", Token::Type::ARROW},
  
  {"+", Token::Type::PLUS},
  {"-", Token::Type::MINUS},
  {"*", Token::Type::MULTIPLY},
  {"/", Token::Type::DIVIDE},
  {"%", Token::Type::MODULO},
  {"=", Token::Type::ASSIGN},
  {"<", Token::Type::LESS},
  {">", Token::Type::GREATER},
  {"(", Token::Type::LPAREN},
  {")", Token::Type::RPAREN},
  {":", Token::Type::COLON},
  {"{", Token::Type::LBRACE},
  {"}", Token::Type::RBRACE},
  {",", Token::Type::COMMA},
  {";", Token::Type::SEMICOLON},
  {"[", Token::Type::LBRACKET},
  {"]", Token::Type::RBRACKET},
  {"@", Token::Type::AT},
  {"?", Token::Type::QMARK},
  {"...", Token::Type::VARDC},
  {"&", Token::Type::AND},
  {"|", Token::Type::OR},
  {"^", Token::Type::XOR},
  {"!", Token::Type::NOT}
};

#endif