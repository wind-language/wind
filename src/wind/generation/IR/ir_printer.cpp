#include <wind/generation/IR.h>
#include <wind/generation/ir_printer.h>
#include <regex>
#include <iostream>

IRPrinter::IRPrinter(IRBody *program) : program(program) {}

const std::unordered_map<IRBinOp::Operation, std::string> BOpTTable = {
  {IRBinOp::Operation::ADD, "add"},
  {IRBinOp::Operation::SUB, "sub"},
  {IRBinOp::Operation::MUL, "mul"},
  {IRBinOp::Operation::DIV, "div"},
  {IRBinOp::Operation::SHL, "shl"},
  {IRBinOp::Operation::SHR, "shr"},
};

IRPrinter::~IRPrinter() {}

void IRPrinter::print() {
  for (auto &node : program->get()) {
    this->print_node(
      dynamic_cast<IRNode *>(node.get())
    );
  }
}

void IRPrinter::print_tabs() {
  std::cout << std::string(this->tabs*2, ' ');
}

void IRPrinter::print_node(const IRNode *node) {
  switch (node->type()) {
    case IRNode::NodeType::FUNCTION : {
      this->print_function(
        node->as<IRFunction>()
      );
      break;
    }
    case IRNode::NodeType::BIN_OP : {
      this->print_bin_op(
        node->as<IRBinOp>()
      );
      break;
    }
    case IRNode::NodeType::LITERAL : {
      this->print_lit(
        node->as<IRLiteral>()
      );
      break;
    }
    case IRNode::NodeType::RET : {
      this->print_ret(node->as<IRRet>());
      break;
    }
    case IRNode::NodeType::LOCAL_REF : {
      this->print_ref(node->as<IRLocalRef>());
      break;
    }
    case IRNode::NodeType::BODY : {
      this->print_body(
        node->as<IRBody>()
      );
      break;
    }
    case IRNode::NodeType::LOCAL_DECL : {
      this->print_ldecl(
        node->as<IRLocalDecl>()
      );
      break;
    }
    case IRNode::NodeType::ARG_DECL : {
      this->print_argdecl(
        node->as<IRArgDecl>()
      );
      break;
    }
    case IRNode::NodeType::FUNCTION_CALL : {
      this->print_fncall(
        node->as<IRFnCall>()
      );
      break;
    }
    case IRNode::NodeType::IN_ASM : {
      this->print_asm(
        node->as<IRInlineAsm>()
      );
      break;
    }
    default : {
      std::cout << "Unknown node type" << std::endl;
      break;
    }
  }
}

void IRPrinter::print_body(const IRBody *body) {
  this->tabs++;
  for (auto &node : body->get()) {
    this->print_node(
      dynamic_cast<IRNode *>(node.get())
    );
  }
  this->tabs--;
}

void IRPrinter::print_function(const IRFunction *node) {
  this->print_tabs();
  std::cout << "function " << node->name() << " {" << std::endl;
  this->print_body(node->body());
  std::cout << "}" << std::endl;
}

void IRPrinter::print_bin_op(const IRBinOp *node) {
  this->in_expr = true;
  if (!node) {
    std::cerr << "Error: IRBinOp node is null" << std::endl;
    return;
  }

  const IRNode* left = node->left();
  const IRNode* right = node->right();

  auto bop = BOpTTable.find(node->operation());
  if (bop == BOpTTable.end()) {
    std::cerr << "Error: Unknown operation" << std::endl;
    return;
  }
  std::cout << bop->second << "";
  std::cout << "(";
  this->print_node(left);
  std::cout << ", ";
  this->print_node(right);
  std::cout << ")";
  this->in_expr = false;
}

void IRPrinter::print_ret(const IRRet *node) {
  this->print_tabs();
  std::cout << "ret ";
  this->print_node(node->get());
  std::cout << std::endl;
}

void IRPrinter::print_ref(const IRLocalRef *node) {
  std::cout << "loc" << node->offset();
}

void IRPrinter::print_lit(const IRLiteral *node) {
  std::cout << node->get();
}

void IRPrinter::print_ldecl(const IRLocalDecl *node) {
  this->print_tabs();
  std::cout << "alloc [" << node->local()->size() << "] loc" << node->local()->offset() << std::endl;
  if (node->value()) {
    this->print_tabs();
    std::cout << "store ";
    this->print_node(node->value());
    std::cout << " -> loc" << node->local()->offset() << std::endl;
  }
}

void IRPrinter::print_argdecl(const IRArgDecl *node) {
  std::cout << "  alloc [" << node->local()->size() << "] loc" << node->local()->offset() << std::endl;
}

void IRPrinter::print_asm(const IRInlineAsm *node) {
  this->print_tabs();
  // replace every \n with \n\t
  std::string code = node->code();
  std::regex newline("\n");
  code = std::regex_replace(code, newline, "\n    ");
  std::cout << "asm {\n    " << code << "\n  }" << std::endl;
}

void IRPrinter::print_fncall(const IRFnCall *node) {
  if (!this->in_expr) {
    this->print_tabs();
  }
  std::cout << "call " << node->name() << "(";
  for (auto &arg : node->args()) {
    this->print_node(arg.get());
    if (&arg != &node->args().back())
      std::cout << ", ";
  }
  std::cout << ")";
  if (!this->in_expr) {
    std::cout << std::endl;
  }
}