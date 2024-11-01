#include <wind/bridge/ast.h>
#include <iostream>

// BinaryExpr

BinaryExpr::BinaryExpr(std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r, std::string o)
  : left(std::move(l)), right(std::move(r)), op(o) {}

void *BinaryExpr::accept(ASTVisitor &visitor) const {
  return visitor.visit(*this);
}

const ASTNode* BinaryExpr::getLeft() const {
  return left.get();
}

const ASTNode* BinaryExpr::getRight() const {
  return right.get();
}

std::string BinaryExpr::getOperator() const {
  return op;
}

// VariableRef

VariableRef::VariableRef(const std::string& n) : name(n) {}

void *VariableRef::accept(ASTVisitor &visitor) const {
  return visitor.visit(*this);
}

const std::string& VariableRef::getName() const {
  return name;
}

// Literal

Literal::Literal(int v) : value(v) {}

void *Literal::accept(ASTVisitor &visitor) const {
  return visitor.visit(*this);
}

int Literal::get() const {
  return value;
}

// Return

Return::Return(std::unique_ptr<ASTNode> v) : value(std::move(v)) {}

void *Return::accept(ASTVisitor &visitor) const {
  return visitor.visit(*this);
}

const ASTNode* Return::get() const {
  return value.get();
}


// Body

void *Body::accept(ASTVisitor &visitor) const {
  return visitor.visit(*this);
}

Body::Body(std::vector<std::unique_ptr<ASTNode>> s) : statements(std::move(s)) {}

const std::vector<std::unique_ptr<ASTNode>>& Body::get() const {
  return statements;
}

Body& Body::operator + (std::unique_ptr<ASTNode> statement) {
  statements.push_back(std::move(statement));
  return *this;
}

Body& Body::operator += (std::unique_ptr<ASTNode> statement) {
  return *this + std::move(statement);
}

// Function

Function::Function(std::string name, std::string type, std::unique_ptr<Body> body) : fn_name(name), return_type(type), body(std::move(body)) {}

void *Function::accept(ASTVisitor &visitor) const {
  return visitor.visit(*this);
}

const std::string& Function::getName() const {
  return fn_name;
}

const Body* Function::getBody() const {
  return body.get();
}

const std::string& Function::getType() const {
  return return_type;
}

LocalDecl::LocalDecl(std::string name, std::string type, std::unique_ptr<ASTNode> value) : name(name), type(type), value(std::move(value)) {}

void *LocalDecl::accept(ASTVisitor &visitor) const {
  return visitor.visit(*this);
}

const std::string& LocalDecl::getName() const {
  return name;
}

const std::string& LocalDecl::getType() const {
  return type;
}

ASTNode *LocalDecl::getValue() const {
  return value.get();
}

FnCall::FnCall(std::string n, std::vector<std::unique_ptr<ASTNode>> a) : name(n), args(std::move(a)) {}

void *FnCall::accept(ASTVisitor &visitor) const {
  return visitor.visit(*this);
}

const std::string& FnCall::getName() const {
  return name;
}

const std::vector<std::unique_ptr<ASTNode>>& FnCall::getArgs() const {
  return args;
}