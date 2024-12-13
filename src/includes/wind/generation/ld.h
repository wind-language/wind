#include <string>
#ifndef LD_H
#define LD_H

class WindLdInterface {
public:
  WindLdInterface();
  void addFlag(std::string flag);
  void addFile(std::string input);
  std::string link();

private:
  std::string files;
  std::string flags;
  int retcode;
};

#endif