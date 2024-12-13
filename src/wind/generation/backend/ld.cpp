#include <wind/generation/ld.h>
#include <wind/common/debug.h>
#include <wind/processing/utils.h>

WindLdInterface::WindLdInterface() {
    this->addFlag("-m elf_x86_64");
    this->addFlag("-dynamic-linker /lib64/ld-linux-x86-64.so.2");
    this->addFlag("-lc");
    this->addFile("raw_std/_start.o");
    this->addFile("raw_std/stack.o");
}

void WindLdInterface::addFlag(std::string flag) {
  this->flags += " " + flag;
}

void WindLdInterface::addFile(std::string input) {
  this->files += " " + input;
}

std::string WindLdInterface::link() {
  std::string output = generateRandomFilePath("", ".out");
  std::string command = "ld" + this->files + " " + this->flags + " -o " + output;
  this->retcode = std::system(command.c_str());
  return output;
}