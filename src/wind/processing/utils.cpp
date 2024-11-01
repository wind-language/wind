#include <wind/processing/utils.h>

bool LexUtils::whitespace(char c) {
  return (
    c == ' ' ||
    c == '\t' ||
    c == '\n' ||
    c == '\r'
  );
}

bool LexUtils::hexadecimal(char c) {
  return (
    (c >= '0' && c <= '9') ||
    (c >= 'a' && c <= 'f') ||
    (c >= 'A' && c <= 'F') ||
    (c == 'x')
  );
}

bool LexUtils::digit(char c) {
  return (c >= '0' && c <= '9');
}

bool LexUtils::alphanum(char c) {
  return (
    (c >= '0' && c <= '9') ||
    (c >= 'a' && c <= 'z') ||
    (c >= 'A' && c <= 'Z')
  );
}