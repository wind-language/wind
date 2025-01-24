#include <string>
#include <stdint.h>
#include <vector>
#include <wind/backend/interface/ld.h>

#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#define EMIT_OBJECT (1 << 0)
#define SHOW_AST    (1 << 1)
#define SHOW_RAW_IR (1 << 2)
#define SHOW_IR     (1 << 3)
#define SHOW_ASM    (1 << 4)

typedef uint16_t EmissionFlags;

class WindUserInterface {
public:
  WindUserInterface(int argc, char **argv);
  ~WindUserInterface();

  void processFiles();
  void emitObject(std::string path, bool canPrint=true);

private:
  void parseArgument(std::string arg, int &i);
  void ldDefFlags(WindLdInterface *ld);
  void ldExecFlags(WindLdInterface *ld);

  std::vector<std::string> files;
  std::string output;
  EmissionFlags flags;
  std::vector<std::string> objects;
  std::vector<std::string> user_ld_flags;
  char **argv;
};

#endif