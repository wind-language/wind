#include <wind/backend/interface/ld.h>
#include <wind/common/debug.h>
#include <wind/processing/utils.h>

WindLdInterface::WindLdInterface(std::string output): output(output) {}

void WindLdInterface::addFlag(std::string flag) {
  this->flags += " " + flag;
}

void WindLdInterface::addFile(std::string input) {
  this->files += " " + input;
}

std::string WindLdInterface::link() {
  if (this->output == "") {
    this->output = generateRandomFilePath("", ".out");
  }
  std::string command = "ld" + this->files + this->flags + " -o " + this->output;
  this->retcode = std::system(command.c_str());
  return this->output;
}