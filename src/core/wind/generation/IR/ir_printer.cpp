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
  {IRBinOp::Operation::AND, "and"},
  {IRBinOp::Operation::L_ASSIGN, "mov"},
  {IRBinOp::Operation::G_ASSIGN, "mov"},
  {IRBinOp::Operation::VA_ASSIGN, "mov"},
  {IRBinOp::Operation::GEN_INDEX_ASSIGN, "mov"},
  {IRBinOp::Operation::EQ, "eq"},
  {IRBinOp::Operation::LESS, "le"},
  {IRBinOp::Operation::LESSEQ, "leq"},
  {IRBinOp::Operation::GREATER, "gr"},
  {IRBinOp::Operation::MOD, "mod"},
  {IRBinOp::Operation::LOGAND, "&&"},
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
      const IRBinOp *bop = node->as<IRBinOp>();
      if (bop->operation() == IRBinOp::Operation::L_ASSIGN) {
        this->print_tabs();
      }
      this->print_bin_op(
        bop
      );
      if (bop->operation() == IRBinOp::Operation::L_ASSIGN) {
        std::cout << std::endl;
      }
      break;
    }
    case IRNode::NodeType::LITERAL : {
      this->print_lit(
        node->as<IRLiteral>()
      );
      break;
    }
    case IRNode::NodeType::STRING : {
      std::cout << "\"" << node->as<IRStringLiteral>()->get() << "\"";
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
    case IRNode::NodeType::LADDR_REF : {
      this->print_laddr(
        node->as<IRLocalAddrRef>()
      );
      break;
    }
    case IRNode::NodeType::GLOBAL_REF : {
      this->print_gref(
        node->as<IRGlobRef>()
      );
      break;
    }
    case IRNode::NodeType::BODY : {
      this->print_body(
        node->as<IRBody>()
      );
      break;
    }
    case IRNode::NodeType::GLOBAL_DECL : {
      this->print_gdecl(
        node->as<IRGlobalDecl>()
      );
      break;
    }
    case IRNode::NodeType::LOCAL_DECL : {
      this->print_ldecl(
        node->as<IRVariableDecl>()
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
    case IRNode::NodeType::BRANCH : {
      this->print_branch(
        node->as<IRBranching>()
      );
      break;
    }
    case IRNode::NodeType::LOOP : {
      this->print_loop(
        node->as<IRLooping>()
      );
      break;
    }
    case IRNode::NodeType::BREAK : {
      std::cout << "break";
      break;
    }
    case IRNode::NodeType::CONTINUE : {
      std::cout << "continue";
      break;
    }
    case IRNode::NodeType::GENERIC_INDEXING : {
      this->print_gen_index(
        node->as<IRGenericIndexing>()
      );
      break;
    }
    case IRNode::NodeType::FN_REF : {
      this->print_fnref(
        node->as<IRFnRef>()
      );
      break;
    }
    case IRNode::NodeType::PTR_GUARD : {
      this->print_ptr_guard(
        node->as<IRPtrGuard>()
      );
      break;
    }
    case IRNode::NodeType::TYPE_CAST : {
      this->print_typecast(
        node->as<IRTypeCast>()
      );
      break;
    }
    case IRNode::NodeType::TRY_CATCH: {
      this->print_trycatch(
        node->as<IRTryCatch>()
      );
      break;
    }
    default : {
      std::cout << "Unknown node type " << (int)node->type() << std::endl;
      break;
    }
  }
}

void IRPrinter::print_body(const IRBody *body) {
  this->tabs++;
  for (auto &node : body->get()) {
    this->print_tabs();
    this->print_node(
      dynamic_cast<IRNode *>(node.get())
    );
    std::cout << std::endl;
  }
  this->tabs--;
}

void IRPrinter::print_function(const IRFunction *node) {
  this->print_tabs();
  if (node->flags & FN_EXTERN) {
    std::cout << "extern " << node->name() << "\n" << std::endl;
    return;
  }
  std::cout << "function " << node->name() << " {" << std::endl;
  this->print_body(node->body());
  std::cout << "}\n" << std::endl;
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
  std::cout << "ret ";
  IRNode *val = node->get();
  if (val) {
    this->print_node(val);
  }
}

void IRPrinter::print_ref(const IRLocalRef *node) {
  std::cout << "loc" << std::abs(node->offset());
}

void IRPrinter::print_laddr(const IRLocalAddrRef *node) {
  std::cout << "&loc" << std::abs(node->offset());
  if (node->isIndexed()) {
    std::cout << "[";
    this->print_node(node->getIndex());
    std::cout << "]";
  }
}

void IRPrinter::print_gref(const IRGlobRef *node) {
  std::cout << "glb " << node->getName();
}

void IRPrinter::print_lit(const IRLiteral *node) {
  std::cout << node->get();
}

void IRPrinter::print_ldecl(const IRVariableDecl *node) {
  std::cout << "alloc [" << node->local()->datatype()->memSize() << "] loc" << std::abs(node->local()->offset());
  if (node->value()) {
    std::cout << "\n";
    this->print_tabs();
    std::cout << "store ";
    this->print_node(node->value());
    std::cout << " -> loc" << std::abs(node->local()->offset());
  }
}

void IRPrinter::print_gdecl(const IRGlobalDecl *node) {
  std::cout << "global " << node->global()->getName() << " [" << node->global()->getType()->memSize() << "]";
  if (node->value()) {
    std::cout << " = ";
    this->print_node(node->value());
  }
}

void IRPrinter::print_argdecl(const IRArgDecl *node) {
  std::cout << "  alloc [" << node->local()->datatype()->memSize() << "] loc" << std::abs(node->local()->offset());
}

void IRPrinter::print_asm(const IRInlineAsm *node) {
  // replace every \n with \n\t
  std::string code = node->code();
  std::regex newline("\n");
  code = std::regex_replace(code, newline, "\n    ");
  std::cout << "asm {\n    " << code << "\n  }";
}

void IRPrinter::print_fncall(const IRFnCall *node) {
  std::cout << "call " << node->name() << "(";
  for (auto &arg : node->args()) {
    this->print_node(arg.get());
    if (&arg != &node->args().back())
      std::cout << ", ";
  }
  std::cout << ")";
}

void IRPrinter::print_branch(const IRBranching *node) {
  std::cout << "branch [\n";
  this->tabs++;
  for (const auto &branch : node->getBranches()) {
    this->print_tabs();
    std::cout << "if ";
    this->print_node(branch.condition.get());
    std::cout << " {\n";
    this->tabs++;
    this->print_node(branch.body.get());
    this->tabs--;
    this->print_tabs();
    std::cout << "}\n";
  }
  if (node->getElseBranch()) {
    this->print_tabs();
    std::cout << "else {\n";
    this->tabs++;
    this->print_node(node->getElseBranch());
    this->tabs--;
    this->print_tabs();
    std::cout << "}\n";
  }
  this->tabs--;
  this->print_tabs();
  std::cout << "]\n";
}

void IRPrinter::print_loop(const IRLooping *node) {
  std::cout << "while ";
  this->print_node(node->getCondition());
  std::cout << " {\n";
  this->tabs++;
  this->print_node(node->getBody());
  this->tabs--;
  this->print_tabs();
  std::cout << "}\n";
}

void IRPrinter::print_gen_index(const IRGenericIndexing *node) {
  this->print_node(node->getBase());
  std::cout << "[";
  this->print_node(node->getIndex());
  std::cout << "]";
}

void IRPrinter::print_fnref(const IRFnRef *node) {
  std::cout << node->name();
}

void IRPrinter::print_ptr_guard(const IRPtrGuard *node) {
  std::cout << "guard[";
  this->print_node(node->getValue());
  std::cout << "]";
}

void IRPrinter::print_typecast(const IRTypeCast *node) {
  std::cout << "<" << node->getType()->rawSize() << ">(";
  this->print_node(node->getValue());
  std::cout << ")";
}

void IRPrinter::print_trycatch(const IRTryCatch *node) {
  std::cout << "try {\n";
  this->print_node(node->getTryBody());
  this->print_tabs();
  std::cout << "}";
  for (auto &handler : node->getHandlerMap()) {
    std::cout << " [h" << (int)handler.first << "] {\n";
    this->tabs++;
    this->print_node(handler.second);
    this->tabs--;
    this->print_tabs();
    std::cout << "}";
  }
  if (node->getFinallyBody()) {
    std::cout << " finally {\n";
    this->tabs++;
    this->print_node(node->getFinallyBody());
    this->tabs--;
    this->print_tabs();
    std::cout << "}";
  }
}