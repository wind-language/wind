#include <wind/generation/IR.h>
#include <wind/generation/backend.h>
#include <asmjit/asmjit.h>
#include <iostream>
#include <iomanip>
#include <fstream>

void WindEmitter::InitializeSections() {
  this->text = this->code_holder.textSection();
  this->code_holder.newSection(
    &this->data, ".data", 6,
    asmjit::SectionFlags::kNone,
    1,
    1
  );
  this->code_holder.newSection(
    &this->bss, ".bss", 5,
    asmjit::SectionFlags::kZeroInitialized,
    1,
    2
  );
  this->code_holder.newSection(
    &this->rodata, ".rodata", 8,
    asmjit::SectionFlags::kReadOnly,
    1,
    3
  );
}

WindEmitter::WindEmitter(IRBody *program) : program(program) {
  this->code_holder.init(asmjit::Environment::host());
  this->InitializeSections();
  this->assembler = new asmjit::x86::Assembler(&this->code_holder);
  FILE *logFile = fopen("output.asm", "w");
  this->logger = new asmjit::FileLogger(logFile);
  this->assembler->setLogger(this->logger);
  this->assembler->section(this->text);
}

WindEmitter::~WindEmitter() {
}

void WindEmitter::emitPrologue() {
  if (
    (current_function->isStack() && current_function->used_offsets.size() > 0)
    || current_function->flags & PURE_LOGUE
  ) {
    this->assembler->push(asmjit::x86::rbp);
    this->assembler->mov(asmjit::x86::rbp, asmjit::x86::rsp);
  }
}

void WindEmitter::emitEpilogue() {
  if (
    (current_function->isStack() && current_function->used_offsets.size() > 0) 
    || current_function->flags & PURE_LOGUE
  ) {
    this->assembler->pop(asmjit::x86::rbp);
  }
  this->assembler->ret();
}

void WindEmitter::emitFunction(IRFunction *fn) {
  current_function = fn;
  //this->assembler->section(this->text);
  this->assembler->bind(this->assembler->newNamedLabel(fn->name().c_str()));
  this->emitPrologue();
  for (auto &statement : fn->body()->get()) {
    this->emitNode(statement.get());
  }
  this->emitEpilogue();
}

asmjit::x86::Gp WindEmitter::adaptReg(asmjit::x86::Gp reg, int size) {
  switch (size) {
    case 1:
      return reg.r8();
    case 2:
      return reg.r16();
    case 4:
      return reg.r32();
    case 8:
      return reg.r64();
    default: {
      throw std::runtime_error("Invalid size");
    }
  } 
}

asmjit::x86::Gp WindEmitter::moveVar(IRLocalRef *local, asmjit::x86::Gp dest) {
  asmjit::x86::Gp reg = adaptReg(dest, local->size());
  this->assembler->mov(reg, asmjit::x86::ptr(asmjit::x86::rbp, -local->offset(), local->size()));
  return reg;
}

void WindEmitter::moveIntoVar(IRLocalRef *local, IRNode *value) {
  if (value->is<IRLiteral>()) {
    IRLiteral *lit = value->as<IRLiteral>();
    this->assembler->mov(
      asmjit::x86::ptr(asmjit::x86::rbp, -local->offset(), local->size()),
      lit->get()
    );
  } else {
    asmjit::x86::Gp reg = this->adaptReg(asmjit::x86::rax, local->size());
    asmjit::x86::Gp src = this->adaptReg(this->emitExpr(value, reg), local->size());
    this->assembler->mov(asmjit::x86::ptr(asmjit::x86::rbp, -local->offset(), local->size()), src);
  }
}

asmjit::x86::Gp WindEmitter::emitBinOp(IRBinOp *bin_op, asmjit::x86::Gp dest) {
  IRNode *right_node = (IRNode*)bin_op->right();
  asmjit::x86::Gp left;
  if (right_node->is<IRBinOp>()) {
    left = this->emitExpr((IRNode*)bin_op->left(), asmjit::x86::r10);
    right_node = new IRRegister(this->emitBinOp(right_node->as<IRBinOp>(), asmjit::x86::rax));
  } else {
    left = this->emitExpr((IRNode*)bin_op->left(), asmjit::x86::rax);
  }

  switch (right_node->type()) {
    case IRNode::NodeType::LITERAL : {
      IRLiteral *lit = right_node->as<IRLiteral>();
      switch (bin_op->operation()) {
        case IRBinOp::Operation::ADD : {
          this->assembler->add(left, lit->get());
          break;
        }
        case IRBinOp::Operation::SUB : {
          this->assembler->sub(left, lit->get());
          break;
        }
        case IRBinOp::Operation::MUL : {
          this->assembler->imul(left, lit->get());
          break;
        }
        case IRBinOp::Operation::DIV : {
          throw std::runtime_error("TODO: Implement division");
          break;
        }
        case IRBinOp::Operation::SHL : {
          this->assembler->shl(left, lit->get());
          break;
        }
        case IRBinOp::Operation::SHR : {
          this->assembler->shr(left, lit->get());
          break;
        }
      }
      if (left != dest) {
        this->assembler->mov(dest, left);
      }
      return left;
    }

    case IRNode::NodeType::FUNCTION_CALL : {
      this->assembler->mov(asmjit::x86::r10, left);
      this->emitExpr(right_node, asmjit::x86::rax);
      switch (bin_op->operation()) {
        case IRBinOp::Operation::ADD : {
          this->assembler->add(asmjit::x86::r10, asmjit::x86::rax);
          break;
        }
        case IRBinOp::Operation::SUB : {
          this->assembler->sub(asmjit::x86::r10, asmjit::x86::rax);
          break;
        }
        case IRBinOp::Operation::MUL : {
          this->assembler->imul(asmjit::x86::r10, asmjit::x86::rax);
          break;
        }
        case IRBinOp::Operation::DIV : {
          throw std::runtime_error("TODO: Implement division");
          break;
        }
        case IRBinOp::Operation::SHL : {
          this->assembler->shl(asmjit::x86::r10, asmjit::x86::rax);
          break;
        }
        case IRBinOp::Operation::SHR : {
          this->assembler->shr(asmjit::x86::r10, asmjit::x86::rax);
          break;
        }
      }
      if (dest.r64() != asmjit::x86::rax) {
        this->assembler->mov(dest, asmjit::x86::rax);
      }
      return dest; // TODO: Return the correct register size
    }

    case IRNode::NodeType::LOCAL_REF : {
      IRLocalRef *local = right_node->as<IRLocalRef>();
      switch (bin_op->operation()) {
        case IRBinOp::Operation::ADD : {
          this->assembler->add(left, asmjit::x86::ptr(asmjit::x86::rbp, -local->offset(), local->size()));
          break;
        }
        case IRBinOp::Operation::SUB : {
          this->assembler->sub(left, asmjit::x86::ptr(asmjit::x86::rbp, -local->offset(), local->size()));
          break;
        }
        case IRBinOp::Operation::MUL : {
          this->assembler->imul(left, asmjit::x86::ptr(asmjit::x86::rbp, -local->offset(), local->size()));
          break;
        }
        case IRBinOp::Operation::DIV : {
          throw std::runtime_error("TODO: Implement division");
          break;
        }
        case IRBinOp::Operation::SHL : {
          this->assembler->shl(left, this->moveVar(local, asmjit::x86::r10));
          break;
        }
        case IRBinOp::Operation::SHR : {
          this->assembler->shr(left, this->moveVar(local, asmjit::x86::r10));
          break;
        }
      }
      if (left != dest) {
        this->assembler->mov(dest, left);
      }
      return left;
    }

    case IRNode::NodeType::REGISTER : {
      asmjit::x86::Gp right = right_node->as<IRRegister>()->reg();
      int biggest_size = std::max(left.size(), right.size());
      left = this->adaptReg(left, biggest_size);
      right = this->adaptReg(right, biggest_size);
      switch (bin_op->operation()) {
        case IRBinOp::Operation::ADD : {
          this->assembler->add(left, right);
          break;
        }
        case IRBinOp::Operation::SUB : {
          this->assembler->sub(left, right);
          break;
        }
        case IRBinOp::Operation::MUL : {
          this->assembler->imul(left, right);
          break;
        }
        case IRBinOp::Operation::DIV : {
          throw std::runtime_error("TODO: Implement division");
          break;
        }
        case IRBinOp::Operation::SHL : {
          this->assembler->shl(left, right);
          break;
        }
        case IRBinOp::Operation::SHR : {
          this->assembler->shr(left, right);
          break;
        }
      }
      if (left != dest) {
        this->assembler->mov(dest, left);
      }
      return left;
    }

    default: {
      std::cout << "Unknown node type" << std::endl;
      break;
    }
  }
  return dest;
}

asmjit::x86::Gp WindEmitter::emitLiteral(IRLiteral *lit, asmjit::x86::Gp dest) {
  long long value = lit->get();
  if (value <= INT8_MAX) {
    this->assembler->mov(dest.r8(), value);
    return dest.r8();
  }
  else if (value <= INT16_MAX) {
    this->assembler->mov(dest.r16(), value);
    return dest.r16();
  }
  else if (value <= INT32_MAX) {
    this->assembler->mov(dest.r32(), value);
    return dest.r32();
  }
  else if (value <= INT64_MAX) {
    this->assembler->mov(dest.r64(), value);
    return dest.r64();
  }
  return dest;
}

asmjit::x86::Gp WindEmitter::emitExpr(IRNode *node, asmjit::x86::Gp dest) {
  switch (node->type()) {

    case IRNode::NodeType::LITERAL : {
      IRLiteral *lit = node->as<IRLiteral>();
      return this->emitLiteral(lit, dest);
    }

    case IRNode::NodeType::LOCAL_REF : {
      IRLocalRef *local = node->as<IRLocalRef>();
      return this->moveVar(local, dest);
    }

    case IRNode::NodeType::FUNCTION_CALL : {
      //IRFnCall *fn_call = node->as<IRFnCall>();
      // TODO: Implement function calls
      //       I will need to resolve the function size and the arguments
      //       and then call the function
      break;
    }

    case IRNode::NodeType::BIN_OP : {
      IRBinOp *bin_op = node->as<IRBinOp>();
      return this->emitBinOp(bin_op, dest);
    }

    default: {
      std::cout << "Unknown node type" << std::endl;
      break;
    }

  }
  return dest;
}

void WindEmitter::emitNode(IRNode *node) {
  switch (node->type()) {
    case IRNode::NodeType::FUNCTION : {
      this->emitFunction(node->as<IRFunction>());
      break;
    }
    case IRNode::NodeType::RET : {
      IRRet *ret = node->as<IRRet>();
      this->emitExpr(ret->get(), asmjit::x86::rax);
      break;
    }
    case IRNode::NodeType::LOCAL_DECL : {
      IRLocalDecl *local_decl = node->as<IRLocalDecl>();
      if (local_decl->value()) {
        this->moveIntoVar(local_decl->local(), local_decl->value());
      }
      break;
    }
    default: {
      std::cout << "Unknown node type" << std::endl;
      break;
    }
  }
}

void WindEmitter::emit() {
  for (auto &statement : this->program->get()) {
    this->emitNode(statement.get());
  }

  this->code_holder.flatten();
  this->code_holder.resolveUnresolvedLinks();
  const uint8_t* data = this->text->data();
  uint size = this->text->bufferSize();

  std::ofstream output("output.bin", std::ios::binary);
  output.write(reinterpret_cast<const char*>(data), size);
  output.close();
}