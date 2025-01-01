#include <wind/backend/interface/gas.h>
#include <wind/common/debug.h>

WindGasInterface::WindGasInterface(std::string path, std::string outpath) : file_path(path), outpath(outpath) {}

void WindGasInterface::addFlag(std::string flag) {
  this->flags += " " + flag;
}

std::string WindGasInterface::assemble() {
  if (this->outpath == "") {
    this->outpath = this->file_path + ".o";
  }
  std::string command = "as " + this->file_path + " -o " + outpath + this->flags;
  this->retcode = std::system(command.c_str());
  std::remove(this->file_path.c_str());
  return outpath;
}