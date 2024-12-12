#include <wind/generation/IR.h>
#include <asmjit/asmjit.h>
#include <map>

#ifndef BACKEND_H
#define BACKEND_H

struct StringTable {
  std::map<std::string, std::string> *table;
};

class WindEmitter {
public:
  WindEmitter(IRBody *program);
  virtual ~WindEmitter();
  std::string emit();

private:
  IRBody *program;
  IRFunction *current_function;
  int cconv_index = 0; // reset on each function call/decl
  int rostring_i=0;

  // Asmjit
  asmjit::x86::Assembler *assembler;
  asmjit::CodeHolder code_holder;
  asmjit::StringLogger *logger;
  asmjit::Section *text;
  asmjit::Section *data;
  asmjit::Section *bss;
  asmjit::Section *rodata;

  // Security
  void secHeader();
  void canaryPrologue();
  void canaryEpilogue();

  // Strings
  StringTable string_table;

  std::string newRoString(std::string str);
  void InitializeSections();
  void emitNode(IRNode *node);
  IRFunction *FindFunction(std::string name);
  void emitFunctionCall(IRFnCall *fn_call);
  void SolveCArg(IRNode *arg, int name);
  void emitFunction(IRFunction *fn);
  void emitPrologue();
  void emitEpilogue();
  asmjit::x86::Gp emitExpr(IRNode *node, asmjit::x86::Gp dest);
  asmjit::x86::Gp moveVar(IRLocalRef *local, asmjit::x86::Gp dest);
  asmjit::x86::Gp adaptReg(asmjit::x86::Gp reg, int size);
  void SolveArg(IRArgDecl *decl);
  void moveIntoVar(IRLocalRef *local, IRNode *value);
  asmjit::x86::Gp emitBinOp(IRBinOp *bin_op, asmjit::x86::Gp dest);
  asmjit::x86::Gp emitLiteral(IRLiteral *lit, asmjit::x86::Gp dest);
};

#endif