#include <wind/generation/IR.h>

#ifndef OPTIMIZER_H
#define OPTIMIZER_H

class WindOptimizer {
public:
  WindOptimizer(IRBody *program);
  virtual ~WindOptimizer();
  IRBody *get();
private:
  IRBody *program;
  IRBody *emission;
  IRFunction *current_fn;

  void optimize();

  void OptimizeBody(IRBody *body, IRFunction *parent);
  IRNode *OptimizeNode(IRNode *node);
  IRNode *OptimizeBinOp(IRBinOp *node);
  IRNode *OptimizeExpr(IRNode *node);
  IRNode *OptimizeLDecl(IRLocalDecl *local_decl);

  IRLiteral *OptimizeConstFold(IRBinOp *node);
};

#endif