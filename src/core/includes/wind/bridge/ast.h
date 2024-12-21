#ifndef AST_H
#define AST_H

#include <string>
#include <memory>
#include <vector>
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
  int16_t index;

public:
  explicit VarAddressing(const std::string& n, int16_t index = -1);

  void *accept(ASTVisitor &visitor) const override;

  // Getter
  const std::string& getName() const;
  const int16_t getIndex() const;
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

  void *accept(ASTVisitor &visitor) const override;

  // Getter
  const std::string& getName() const;
  const Body* getBody() const;
  const std::string& getType() const;
  void copyArgTypes(std::vector<std::string> &types);
  std::vector<std::string> getArgTypes() const;

  FnFlags flags;
};

class LocalDecl : public ASTNode {
  std::string name;
  std::string type; // Will resolve on IR generation
  std::unique_ptr<ASTNode> value;

public:
  LocalDecl(std::string n, std::string t, std::unique_ptr<ASTNode> v = nullptr);

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
  virtual void *visit(const LocalDecl &node) = 0;
  virtual void *visit(const FnCall &node) = 0;
  virtual void *visit(const InlineAsm &node) = 0;
  virtual void *visit(const StringLiteral &node) = 0;
  virtual void *visit(const TypeDecl &node) = 0;
  virtual void *visit(const Branching &node) = 0;
  virtual void *visit(const Looping &node) = 0;
};

#endif // AST_H