#include <string>
#include <stdint.h>
#include <vector>
#include <wind/generation/ld.h>

#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#define EMIT_OBJECT (1 << 0)
#define SHOW_AST    (1 << 1)
#define SHOW_IR     (1 << 2)
#define SHOW_ASM    (1 << 3)

typedef uint16_t EmissionFlags;

class WindUserInterface {
public:
  WindUserInterface(int argc, char **argv);
  ~WindUserInterface();

  void processFiles();
  void emitObject(std::string path);

private:
  void parseArgument(std::string arg, int &i);
  void ldDefFlags(WindLdInterface *ld);
  void ldExecFlags(WindLdInterface *ld);

  std::vector<std::string> files;
  std::string output;
  EmissionFlags flags;
  std::vector<std::string> objects;
  char **argv;
};

#endif