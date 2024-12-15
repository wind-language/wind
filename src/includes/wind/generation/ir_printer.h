#include <wind/generation/IR.h>

#ifndef IR_PRINTER_H
#define IR_PRINTER_H

class IRPrinter {
public:
  IRPrinter(IRBody *program);
  virtual ~IRPrinter();
  void print();
private:
  IRBody *program;
  unsigned tabs = 0;
  bool in_expr=false;

  void print_node(const IRNode *node);
  void print_body(const IRBody *body);
  void print_function(const IRFunction *node);
  void print_bin_op(const IRBinOp *node);
  void print_ret(const IRRet *node);
  void print_ref(const IRLocalRef *node);
  void print_laddr(const IRLocalAddrRef *node);
  void print_lit(const IRLiteral *node);
  void print_ldecl(const IRLocalDecl *node);
  void print_argdecl(const IRArgDecl *node);
  void print_fncall(const IRFnCall *node);
  void print_asm(const IRInlineAsm *node);

  void print_tabs();
};

#endif