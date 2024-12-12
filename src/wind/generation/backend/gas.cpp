#include <wind/generation/gas.h>
#include <wind/common/debug.h>

WindGasInterface::WindGasInterface(std::string path) : file_path(path) {}

void WindGasInterface::addFlag(std::string flag) {
  this->flags += " " + flag;
}

std::string WindGasInterface::assemble() {
  std::string obj_path = this->file_path + ".o";
  std::string command = "as " + this->file_path + " -o " + obj_path + this->flags;
  this->retcode = std::system(command.c_str());
  //std::remove(this->file_path.c_str());
  return obj_path;
}