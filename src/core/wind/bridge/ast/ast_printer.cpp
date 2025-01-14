#include <wind/bridge/ast_printer.h>
#include <iostream>
#include <regex>

void ASTPrinter::print_tabs() {
  std::cout << std::string(this->tabs, ' ');
}

void *ASTPrinter::visit(const BinaryExpr &node) {
  this->in_expr=true;
  std::cout << "(";
  node.getLeft()->accept(*this);
  std::cout << " " << node.getOperator() << " ";
  node.getRight()->accept(*this);
  std::cout << ")";
  this->in_expr=false;
  return nullptr;
}

void *ASTPrinter::visit(const VariableRef &node) {
  std::cout << node.getName();
  return nullptr;
}

void *ASTPrinter::visit(const VarAddressing &node) {
  if (node.getIndex()) {
    std::cout << node.getName() << "[";
    node.getIndex()->accept(*this);
    std::cout << "]";
    return nullptr;
  } else {
    std::cout << "&" << node.getName();
  }
  return nullptr;
}

void *ASTPrinter::visit(const Literal &node) {
  std::cout << node.get();
  return nullptr;
}

void *ASTPrinter::visit(const Return &node) {
  std::cout << "return ";
  const ASTNode *val = node.get();
  if (val) {
    val->accept(*this);
  }
  return nullptr;
}

void *ASTPrinter::visit(const Body &node) {
  this->tabs++;
  for (const auto &child : node.get()) {
    this->print_tabs();
    child->accept(*this);
    std::cout << std::endl;
  }
  this->tabs--;
  return nullptr;
}

void *ASTPrinter::visit(const Function &node) {
  std::cout << "def " << node.getName() << std::endl;
  node.getBody()->accept(*this);
  return nullptr;
}

void *ASTPrinter::visit(const VariableDecl &node) {
  std::cout << "decl [" << node.getType() << "] " << node.getNames()[0];
  if (node.getValue()) {
    std::cout << " = ";
    node.getValue()->accept(*this);
  }
  return nullptr;
}

void *ASTPrinter::visit(const ArgDecl &node) {
  std::cout << "arg [" << node.getType() << "] " << node.getName();
  return nullptr;
}

void *ASTPrinter::visit(const InlineAsm &node) {
  std::string code = node.getCode();
  std::regex newline("\n");
  code = std::regex_replace(code, newline, "\n    ");
  std::cout << "asm {\n    " << code << "\n  }";
  return nullptr;
}

void *ASTPrinter::visit(const FnCall &node) {
  std::cout << node.getName() << "(";
  for (const auto &arg : node.getArgs()) {
    arg->accept(*this);
    if (&arg != &node.getArgs().back())
      std::cout << ", ";
  }
  std::cout << ")";
  return nullptr;
}

void *ASTPrinter::visit(const StringLiteral &node) {
  std::cout << "\"" << node.getValue() << "\"";
  return nullptr;
}

void *ASTPrinter::visit(const TypeDecl &node) {
  std::cout << "type " << node.getName() << " = " << node.getType() << std::endl;
  return nullptr;
}

void *ASTPrinter::visit(const Branching &node) {
  std::cout << "branch [\n";
  this->tabs++;
  for (const auto &branch : node.getBranches()) {
    this->print_tabs();
    std::cout << "if ";
    branch.condition->accept(*this);
    std::cout << " {\n";
    this->tabs++;
    branch.body->accept(*this);
    this->tabs--;
    this->print_tabs();
    std::cout << "}\n";
  }
  if (node.getElseBranch()) {
    this->print_tabs();
    std::cout << "else {\n";
    this->tabs++;
    node.getElseBranch()->accept(*this);
    this->tabs--;
    this->print_tabs();
    std::cout << "}\n";
  }
  this->tabs--;
  this->print_tabs();
  std::cout << "]\n";
  return nullptr;
}

void *ASTPrinter::visit(const Looping &node) {
  std::cout << "loop [";
  node.getCondition()->accept(*this);
  std::cout << "] {\n";
  this->tabs++;
  node.getBody()->accept(*this);
  this->tabs--;
  this->print_tabs();
  std::cout << "}\n";
  return nullptr;
}

void *ASTPrinter::visit(const GlobalDecl &node) {
  std::cout << "global " << node.getName() << " [" << node.getType() << "]" << std::endl;
  return nullptr;
}

void *ASTPrinter::visit(const Break &node) {
  std::cout << "break";
  return nullptr;
}

void *ASTPrinter::visit(const Continue &node) {
  std::cout << "continue";
  return nullptr;
}

void *ASTPrinter::visit(const GenericIndexing &node) {
  node.getBase()->accept(*this);
  std::cout << "[";
  node.getIndex()->accept(*this);
  std::cout << "]";
  return nullptr;
}

void *ASTPrinter::visit(const PtrGuard &node) {
  this->print_tabs();
  std::cout << "guard ";
  node.getValue()->accept(*this);
  return nullptr;
}