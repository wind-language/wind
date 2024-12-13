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

void *ASTPrinter::visit(const Literal &node) {
  std::cout << node.get();
  return nullptr;
}

void *ASTPrinter::visit(const Return &node) {
  this->print_tabs();
  std::cout << "return\n";
  this->tabs++;
  this->print_tabs();
  node.get()->accept(*this);
  this->tabs--;
  return nullptr;
}

void *ASTPrinter::visit(const Body &node) {
  this->tabs++;
  for (const auto &child : node.get()) {
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

void *ASTPrinter::visit(const LocalDecl &node) {
  this->print_tabs();
  std::cout << "decl [" << node.getType() << "] " << node.getName();
  if (node.getValue()) {
    std::cout << " = ";
    node.getValue()->accept(*this);
  }
  return nullptr;
}

void *ASTPrinter::visit(const ArgDecl &node) {
  this->print_tabs();
  std::cout << "arg [" << node.getType() << "] " << node.getName();
  return nullptr;
}

void *ASTPrinter::visit(const InlineAsm &node) {
  this->print_tabs();
  std::string code = node.getCode();
  std::regex newline("\n");
  code = std::regex_replace(code, newline, "\n    ");
  std::cout << "asm {\n    " << code << "\n  }";
  return nullptr;
}

void *ASTPrinter::visit(const FnCall &node) {
  if (!this->in_expr) {
    this->print_tabs();
  }
  std::cout << node.getName() << "(";
  for (const auto &arg : node.getArgs()) {
    arg->accept(*this);
    if (&arg != &node.getArgs().back())
      std::cout << ", ";
  }
  std::cout << ")";
  return nullptr;
}