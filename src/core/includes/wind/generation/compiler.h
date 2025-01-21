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
  std::map<std::string, IRFunction*> fn_table;
  bool decl_return = false;
  std::vector<std::string> active_namespaces;

  std::string nameMangle(const std::string &name) {
    std::string mangled = "";
    for (char c : name) {
      if (c == ':') {
        mangled += "_";
      } else {
        mangled += c;
      }
    }
    return mangled;
  }
  std::string getFullMangled(const std::string &name) {
    std::string base = "";
    for (const std::string &ns : active_namespaces) {
      base += ns + "__";
    }
    base += nameMangle(name);
    return base;
  }
  std::map<std::string, IRFunction *>::iterator findFunction(const std::string &name) {
    std::string base = nameMangle(name);
    auto found = fn_table.find(base);
    if (found != fn_table.end()) {
      return found;
    }
    for (const std::string &ns : active_namespaces) {
      base = ns + "__" + base;
      found = fn_table.find(base);
      if (found != fn_table.end()) {
        return found;
      }
    }
    return fn_table.end();
  }


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