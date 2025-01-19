#include <wind/generation/IR.h>
#include <map>

#ifndef OPTIMIZER_H
#define OPTIMIZER_H

/**
 * @brief A class to optimize the intermediate representation (IR) of a program.
 */
class WindOptimizer {
public:
  /**
   * @brief Constructor for WindOptimizer.
   * @param program The IRBody representing the program to be optimized.
   */
  WindOptimizer(IRBody *program);

  /**
   * @brief Destructor for WindOptimizer.
   */
  virtual ~WindOptimizer();

  /**
   * @brief Gets the optimized IRBody.
   * @return The optimized IRBody.
   */
  IRBody *get();

private:
  IRBody *program;
  IRBody *emission;
  struct FunctionDesc {
    IRFunction *fn;
    std::map<int16_t, IRNode*> lkv; // local known values
  } *current_fn=nullptr;

  /**
   * @brief Runs the optimization process on the program.
   */
  void optimize();

  /**
   * @brief Optimizes the body of a function.
   * @param body The body to be optimized.
   * @param parent The parent function of the body.
   */
  void OptimizeBody(IRBody *body, IRFunction *parent);

  /**
   * @brief Optimizes a generic node.
   * @param node The node to be optimized.
   * @return An optimized IRNode.
   */
  IRNode *OptimizeNode(IRNode *node, bool canLocFold);

  /**
   * @brief Optimizes binary operations.
   * @param node The binary operation node to be optimized.
   * @return An optimized IRNode.
   */
  IRNode *OptimizeBinOp(IRBinOp *node, bool canLocFold);

  /**
   * @brief Optimizes expressions.
   * @param node The expression node to be optimized.
   * @return An optimized IRNode.
   */
  IRNode *OptimizeExpr(IRNode *node, bool canLocFold);

  /**
   * @brief Optimizes local variable declarations.
   * @param local_decl The local variable declaration node to be optimized.
   * @return An optimized IRNode.
   */
  IRNode *OptimizeLDecl(IRVariableDecl *local_decl, bool canLocFold);

  /**
   * @brief Optimizes function calls.
   * @param fn_call The function call node to be optimized.
   * @return An optimized IRNode.
   */
  IRNode *OptimizeFnCall(IRFnCall *fn_call, bool canLocFold);

  /**
   * @brief Optimizes functions.
   * @param fn The function node to be optimized.
   * @return An optimized IRNode.
   */
  IRNode *OptimizeFunction(IRFunction *fn);

  /**
   * @brief Optimizes branching statements.
   * @param branch The branching node to be optimized.
   * @return An optimized IRNode.
   */
  IRNode *OptimizeBranching(IRBranching *branch);

  /**
   * @brief Optimizes looping statements.
   * @param loop The looping node to be optimized.
   * @return An optimized IRNode.
   */
  IRNode *OptimizeLooping(IRLooping *loop);

  IRNode *OptimizeTryCatch(IRTryCatch *try_catch, bool canLocFold);

  /**
   * @brief Optimizes constant folding for binary operations.
   * @param node The binary operation node to be optimized.
   * @return A new IRLiteral node with the optimized result.
   */
  IRLiteral *OptimizeConstFold(IRBinOp *node);

  IRNode *OptimizeGenIndexing(IRGenericIndexing *indexing, bool canLocFold);
  IRNode *OptimizePtrGuard(IRPtrGuard *ptr_guard, bool canLocFold);
  IRNode *OptimizeTypeCast(IRTypeCast *type_cast, bool canLocFold);

  void NewLocalValue(int16_t local, IRNode *value);
  IRNode *GetLocalValue(int16_t local);
};

#endif