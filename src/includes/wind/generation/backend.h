#include <wind/generation/IR.h>
#include <asmjit/asmjit.h>

#ifndef BACKEND_H
#define BACKEND_H

class WindEmitter {
public:
  WindEmitter(IRBody *program);
  virtual ~WindEmitter();
  void emit();

private:
  IRBody *program;
  IRFunction *current_function;

  // Asmjit
  asmjit::x86::Assembler *assembler;
  asmjit::CodeHolder code_holder;
  asmjit::FileLogger *logger;
  asmjit::Section *text;
  asmjit::Section *data;
  asmjit::Section *bss;
  asmjit::Section *rodata;

  void InitializeSections();
  void emitNode(IRNode *node);

  void emitFunction(IRFunction *fn);
  void emitPrologue();
  void emitEpilogue();
  asmjit::x86::Gp emitExpr(IRNode *node, asmjit::x86::Gp dest);
  asmjit::x86::Gp moveVar(IRLocalRef *local, asmjit::x86::Gp dest);
  asmjit::x86::Gp adaptReg(asmjit::x86::Gp reg, int size);
  void moveIntoVar(IRLocalRef *local, IRNode *value);
  asmjit::x86::Gp emitBinOp(IRBinOp *bin_op, asmjit::x86::Gp dest);
  asmjit::x86::Gp emitLiteral(IRLiteral *lit, asmjit::x86::Gp dest);
};

#endif