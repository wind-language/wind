#ifndef AST_H
#define AST_H

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <wind/bridge/opt_flags.h>

class ASTVisitor;

class ASTNode {
public:
  virtual ~ASTNode() = default;
  virtual void *accept(ASTVisitor &visitor) const = 0;
};

class BinaryExpr : public ASTNode {
  std::unique_ptr<ASTNode> left;
  std::unique_ptr<ASTNode> right;
  std::string op;

public:
  BinaryExpr(std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r, std::string o);

  void *accept(ASTVisitor &visitor) const override;

  // Getters
  const ASTNode* getLeft() const;
  const ASTNode* getRight() const;
  std::string getOperator() const;
};

class VariableRef : public ASTNode {
  std::string name;

public:
  explicit VariableRef(const std::string& n);

  void *accept(ASTVisitor &visitor) const override;

  // Getter
  const std::string& getName() const;
};

class VarAddressing : public ASTNode {
  std::string name;
  ASTNode *index;

public:
  explicit VarAddressing(const std::string& n, ASTNode *index = nullptr);

  void *accept(ASTVisitor &visitor) const override;

  // Getter
  const std::string& getName() const;
  const ASTNode *getIndex() const;
};

class Literal : public ASTNode {
  long long value;

public:
  explicit Literal(long long v);

  void *accept(ASTVisitor &visitor) const override;

  // Getter
  int get() const;
};

class Return : public ASTNode {
  std::unique_ptr<ASTNode> value;

public:
  explicit Return(std::unique_ptr<ASTNode> v);

  void *accept(ASTVisitor &visitor) const override;

  // Getter
  const ASTNode* get() const;
};

class Body : public ASTNode {
  std::vector<std::unique_ptr<ASTNode>> statements;

public:
  std::map<std::string, ASTNode*> consts_table;

  explicit Body(std::vector<std::unique_ptr<ASTNode>> s);

  Body(const Body&) = delete;
  Body& operator=(const Body&) = delete;

  Body(Body&&) noexcept = default;
  Body& operator=(Body&&) noexcept = default;

  void *accept(ASTVisitor &visitor) const override;

  const std::vector<std::unique_ptr<ASTNode>>& get() const;
  Body& operator + (std::unique_ptr<ASTNode> statement);
  Body& operator += (std::unique_ptr<ASTNode> statement);
  Body& operator + (Body&) = delete;
};

class Function : public ASTNode {
  std::string fn_name;
  std::string return_type;
  std::unique_ptr<Body> body;
  std::vector<std::string> arg_types;

public:
  Function(std::string name, std::string type, std::unique_ptr<Body> b);
  std::string metadata="";
  bool isDefined = true;

  void *accept(ASTVisitor &visitor) const override;

  // Getter
  const std::string& getName() const;
  const Body* getBody() const;
  const std::string& getType() const;
  void copyArgTypes(std::vector<std::string> &types);
  std::vector<std::string> getArgTypes() const;

  FnFlags flags;
};

class VariableDecl : public ASTNode {
  std::vector<std::string> vars;
  std::string type; // Will resolve on IR generation
  std::unique_ptr<ASTNode> value;

public:
  VariableDecl(std::vector<std::string> n, std::string t, std::unique_ptr<ASTNode> v = nullptr);

  void *accept(ASTVisitor &visitor) const override;

  // Getters
  const std::vector<std::string>& getNames() const;
  const std::string& getType() const;
  ASTNode *getValue() const;
};

class GlobalDecl : public ASTNode {
  std::string name;
  std::string type; // Will resolve on IR generation
  std::unique_ptr<ASTNode> value;

public:
  GlobalDecl(std::string n, std::string t, std::unique_ptr<ASTNode> v = nullptr);

  void *accept(ASTVisitor &visitor) const override;

  // Getters
  const std::string& getName() const;
  const std::string& getType() const;
  ASTNode *getValue() const;
};

class ArgDecl : public ASTNode {
  std::string name;
  std::string type; // Will resolve on IR generation

public:
  ArgDecl(std::string n, std::string t);

  void *accept(ASTVisitor &visitor) const override;

  // Getters
  const std::string& getName() const;
  const std::string& getType() const;
};

class FnCall : public ASTNode {
  std::string name;

public:
  std::vector<std::unique_ptr<ASTNode>> args;
  FnCall(std::string n, std::vector<std::unique_ptr<ASTNode>> a);

  void *accept(ASTVisitor &visitor) const override;

  // Getters
  const std::string& getName() const;
  const std::vector<std::unique_ptr<ASTNode>>& getArgs() const;
};

class InlineAsm : public ASTNode {
  std::string code;

public:
  InlineAsm(std::string c);
  void *accept(ASTVisitor &visitor) const override;
  const std::string& getCode() const;
};

class StringLiteral : public ASTNode {
public:
  StringLiteral(std::string c);
  void *accept(ASTVisitor &visitor) const;
  const std::string& getValue() const;

private:
  std::string str;
};

class TypeDecl : public ASTNode {
  std::string name;
  std::string type;

public:
  TypeDecl(std::string n, std::string t);
  void *accept(ASTVisitor &visitor) const;
  const std::string& getName() const;
  const std::string& getType() const;
};

struct Branch {
  std::unique_ptr<ASTNode> condition;
  std::unique_ptr<Body> body;
};

class Branching : public ASTNode {
  std::vector<Branch> branches;
  Body* else_branch;

public:
  void *accept(ASTVisitor &visitor) const;
  const std::vector<Branch>& getBranches() const;
  const Body *getElseBranch() const;
  void setElseBranch(Body* body);
  void addBranch(std::unique_ptr<ASTNode> condition, std::unique_ptr<Body> body);
};

class Looping : public ASTNode {
  ASTNode* condition;
  Body* body;

public:
  void *accept(ASTVisitor &visitor) const;
  void setCondition(ASTNode* c);
  const ASTNode* getCondition() const;
  void setBody(Body* b);
  const Body* getBody() const;
};

class Break : public ASTNode {
public:
  Break();
  void *accept(ASTVisitor &visitor) const;
};

class Continue : public ASTNode {
public:
  Continue();
  void *accept(ASTVisitor &visitor) const;
};

class GenericIndexing : public ASTNode {
  std::unique_ptr<ASTNode> index;
  std::unique_ptr<ASTNode> base;

public:
  GenericIndexing(std::unique_ptr<ASTNode> i, std::unique_ptr<ASTNode> b);
  void *accept(ASTVisitor &visitor) const;
  const ASTNode* getIndex() const;
  const ASTNode* getBase() const;
};

class PtrGuard : public ASTNode {
  std::unique_ptr<ASTNode> value;

public:
  PtrGuard(std::unique_ptr<ASTNode> v);
  void *accept(ASTVisitor &visitor) const;
  const ASTNode* getValue() const;
};


class ASTVisitor {
public:
  virtual void *visit(const BinaryExpr &node) = 0;
  virtual void *visit(const VariableRef &node) = 0;
  virtual void *visit(const VarAddressing &node) = 0;
  virtual void *visit(const Literal &node) = 0;
  virtual void *visit(const Return &node) = 0;
  virtual void *visit(const Body &node) = 0;
  virtual void *visit(const Function &node) = 0;
  virtual void *visit(const ArgDecl &node) = 0;
  virtual void *visit(const VariableDecl &node) = 0;
  virtual void *visit(const GlobalDecl &node) = 0;
  virtual void *visit(const FnCall &node) = 0;
  virtual void *visit(const InlineAsm &node) = 0;
  virtual void *visit(const StringLiteral &node) = 0;
  virtual void *visit(const TypeDecl &node) = 0;
  virtual void *visit(const Branching &node) = 0;
  virtual void *visit(const Looping &node) = 0;
  virtual void *visit(const Break &node) = 0;
  virtual void *visit(const Continue &node) = 0;
  virtual void *visit(const GenericIndexing &node) = 0;
  virtual void *visit(const PtrGuard &node) = 0;
};

#endif // AST_H