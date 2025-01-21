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

// VarAddressing

VarAddressing::VarAddressing(const std::string& n, ASTNode *index) : name(n), index(index) {}

void *VarAddressing::accept(ASTVisitor &visitor) const {
  return visitor.visit(*this);
}

const std::string& VarAddressing::getName() const {
  return name;
}

const ASTNode *VarAddressing::getIndex() const {
  return index;
}

// Literal

Literal::Literal(long long v) : value(v) {}

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

void Function::copyArgTypes(std::vector<std::string>& types) {
  arg_types = types;
}

std::vector<std::string> Function::getArgTypes() const {
  return arg_types;
}

VariableDecl::VariableDecl(std::vector<std::string> names, std::string type, std::unique_ptr<ASTNode> value) : vars(names), type(type), value(std::move(value)) {}

void *VariableDecl::accept(ASTVisitor &visitor) const {
  return visitor.visit(*this);
}

const std::vector<std::string>& VariableDecl::getNames() const {
  return vars;
}

const std::string& VariableDecl::getType() const {
  return type;
}

ASTNode *VariableDecl::getValue() const {
  return value.get();
}

GlobalDecl::GlobalDecl(std::string name, std::string type, std::unique_ptr<ASTNode> value) : name(name), type(type), value(std::move(value)) {}

void *GlobalDecl::accept(ASTVisitor &visitor) const {
  return visitor.visit(*this);
}

const std::string& GlobalDecl::getName() const {
  return name;
}

const std::string& GlobalDecl::getType() const {
  return type;
}

ASTNode *GlobalDecl::getValue() const {
  return value.get();
}


ArgDecl::ArgDecl(std::string name, std::string type) : name(name), type(type) {}

void *ArgDecl::accept(ASTVisitor &visitor) const {
  return visitor.visit(*this);
}

const std::string& ArgDecl::getName() const {
  return name;
}

const std::string& ArgDecl::getType() const {
  return type;
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

InlineAsm::InlineAsm(std::string c) : code(c) {}
void *InlineAsm::accept(ASTVisitor &visitor) const {
  return visitor.visit(*this);
}
const std::string& InlineAsm::getCode() const {
  return code;
}

StringLiteral::StringLiteral(std::string s) : str(s) {}
void *StringLiteral::accept(ASTVisitor &visitor) const {
  return visitor.visit(*this);
}
const std::string& StringLiteral::getValue() const {
  return str;
}

TypeDecl::TypeDecl(std::string n, std::string t) : name(n), type(t) {}
void *TypeDecl::accept(ASTVisitor &visitor) const {
  return visitor.visit(*this);
}
const std::string& TypeDecl::getName() const {
  return name;
}
const std::string& TypeDecl::getType() const {
  return type;
}

void *Branching::accept(ASTVisitor &visitor) const {
  return visitor.visit(*this);
}
const std::vector<Branch>& Branching::getBranches() const {
  return branches;
}
const Body* Branching::getElseBranch() const {
  return else_branch;
}
void Branching::addBranch(std::unique_ptr<ASTNode> condition, std::unique_ptr<Body> body) {
  branches.push_back({std::move(condition), std::move(body)});
}
void Branching::setElseBranch(Body* body) {
  else_branch = body;
}

void *Looping::accept(ASTVisitor &visitor) const {
  return visitor.visit(*this);
}
void Looping::setCondition(ASTNode* c) {
  condition = c;
}
void Looping::setBody(Body* b) {
  body = b;
}
const Body* Looping::getBody() const {
  return body;
}
const ASTNode* Looping::getCondition() const {
  return condition;
}

Break::Break() {}
void *Break::accept(ASTVisitor &visitor) const {
  return visitor.visit(*this);
}

Continue::Continue() {}
void *Continue::accept(ASTVisitor &visitor) const {
  return visitor.visit(*this);
}

GenericIndexing::GenericIndexing(std::unique_ptr<ASTNode> i, std::unique_ptr<ASTNode> b) {
  index = std::move(i);
  base = std::move(b);
}
void *GenericIndexing::accept(ASTVisitor &visitor) const {
  return visitor.visit(*this);
}
const ASTNode *GenericIndexing::getIndex() const {
  return index.get();
}
const ASTNode *GenericIndexing::getBase() const {
  return base.get();
}

PtrGuard::PtrGuard(std::unique_ptr<ASTNode> v) : value(std::move(v)) {}
void *PtrGuard::accept(ASTVisitor &visitor) const {
  return visitor.visit(*this);
}
const ASTNode *PtrGuard::getValue() const {
  return value.get();
}

TypeCast::TypeCast(std::string t, std::unique_ptr<ASTNode> v) : type(t), value(std::move(v)) {}
void *TypeCast::accept(ASTVisitor &visitor) const {
  return visitor.visit(*this);
}
const std::string &TypeCast::getType() const {
  return type;
}
const ASTNode *TypeCast::getValue() const {
  return value.get();
}

SizeOf::SizeOf(std::string t) : type(t) {}
void *SizeOf::accept(ASTVisitor &visitor) const {
  return visitor.visit(*this);
}
const std::string &SizeOf::getType() const {
  return type;
}


TryCatch::TryCatch() {}
void TryCatch::setTryBody(Body* b) {
  try_body = b;
}
void TryCatch::addCatchBlock(std::string e, Body* b) {
  catch_blocks.push_back({e, b});
}
void TryCatch::setFinallyBlock(Body* b) {
  finally_block = b;
}
void *TryCatch::accept(ASTVisitor &visitor) const {
  return visitor.visit(*this);
}
const Body* TryCatch::getTryBody() const {
  return try_body;
}
const std::vector<std::pair<std::string, Body*>>& TryCatch::getCatchBlocks() const {
  return catch_blocks;
}
const Body* TryCatch::getFinallyBlock() const {
  return finally_block;
}

Namespace::Namespace(std::string n, Body *c) : name(n), children(std::move(c)) {}
void *Namespace::accept(ASTVisitor &visitor) const {
  return visitor.visit(*this);
}
const std::string& Namespace::getName() const {
  return name;
}
Body* Namespace::getChildren() const {
  return children;
}
