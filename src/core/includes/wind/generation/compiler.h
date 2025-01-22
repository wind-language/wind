#include <map>
#include <string>
#include <vector>

#ifndef COMPILER_H
#define COMPILER_H

#include <wind/generation/IR.h>
#include <wind/bridge/ast.h>

// the compiler goal is to take a specific AST and generate a generic IR
class WindCompiler : public ASTVisitor {
public:
  WindCompiler(Body *program);
  virtual ~WindCompiler();
  IRBody *get();

private:
  Body *program;
  IRBody *emission;
  IRFunction *current_fn;
  std::map<std::string, DataType*> userdef_types_map;
  std::map<std::string, IRGlobRef*> global_table;
  bool decl_return = false;
  std::vector<std::string> active_namespaces;
  std::multimap<std::string, IRFunction*> fn_table;


  std::string namespacePrefix(std::string name);
  std::string generalizeType(DataType *type);
  std::string functionSignature(std::string name, DataType *ret, std::vector<DataType*> &args);
  std::pair<std::string, std::string> fnSignHash(std::string name, DataType *ret, std::vector<DataType*> &args);

  bool typeMatch(DataType *left, IRNode *right);
  IRFunction* matchFunction(std::string name, std::vector<std::unique_ptr<IRNode>> &args, bool raise=true);


  void compile();
  DataType *ResolveDataType(const std::string &type);
  void CanCoerce(IRNode *left, IRNode *right);
  void NodeIntoBody(IRNode *node, IRBody *body);

  // Visitor
  void *visit(const BinaryExpr &node) override;
  void *visit(const VariableRef &node) override;
  void *visit(const VarAddressing &node) override;
  void *visit(const Literal &node) override;
  void *visit(const Return &node) override;
  void *visit(const Body &node) override;
  void *visit(const Function &node) override;
  void *visit(const ArgDecl &node) override;
  void *visit(const VariableDecl &node) override;
  void *visit(const GlobalDecl &node) override;
  void *visit(const FnCall &node) override;
  void *visit(const InlineAsm &node) override;
  void *visit(const StringLiteral &node) override;
  void *visit(const TypeDecl &node) override;
  void *visit(const Branching &node) override;
  void *visit(const Looping &node) override;
  void *visit(const Break &node) override;
  void *visit(const Continue &node) override;
  void *visit(const GenericIndexing &node) override;
  void *visit(const PtrGuard &node) override;
  void *visit(const TypeCast &node) override;
  void *visit(const SizeOf &node) override;
  void *visit(const TryCatch &node) override;
  void *visit(const Namespace &node) override;
};

#endif // COMPILER_H