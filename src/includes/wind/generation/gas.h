#include <string>
#ifndef GAS_H
#define GAS_H

class WindGasInterface {
public:
  WindGasInterface(std::string path);
  void addFlag(std::string flag);
  std::string assemble();

private:
  std::string file_path;
  std::string flags;
  int retcode;
};

#endif