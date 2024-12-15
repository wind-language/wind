#include <string>
#ifndef UTILS_H
#define UTILS_H

namespace LexUtils {
  bool whitespace(char c);
  bool hexadecimal(char c);
  bool digit(char c);
  bool alphanum(char c);
}

std::string generateRandomFilePath(const std::string& directory, const std::string& extension);

long long fmtinttostr(std::string &str);

#endif