#include <wind/generation/backend.h>
#include <asmjit/asmjit.h>
#include <iostream>

asmjit::x86::Gp WindEmitter::moveVar(IRLocalRef *local, asmjit::x86::Gp dest) {
  asmjit::x86::Gp reg = adaptReg(dest, local->size());
  if (this->OptVarGet(local->offset()) == reg) {
    return reg;
  }
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
    this->OptVarMoved(local->offset(), src);
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
      asmjit::x86::Gp rax_sz = this->emitExpr(right_node, asmjit::x86::rax);
      switch (bin_op->operation()) {
        case IRBinOp::Operation::ADD : {
          this->assembler->add(this->adaptReg(asmjit::x86::r10, rax_sz.size()), rax_sz);
          break;
        }
        case IRBinOp::Operation::SUB : {
          this->assembler->sub(this->adaptReg(asmjit::x86::r10, rax_sz.size()), rax_sz);
          break;
        }
        case IRBinOp::Operation::MUL : {
          this->assembler->imul(this->adaptReg(asmjit::x86::r10, rax_sz.size()), rax_sz);
          break;
        }
        case IRBinOp::Operation::DIV : {
          throw std::runtime_error("TODO: Implement division");
          break;
        }
        case IRBinOp::Operation::SHL : {
          this->assembler->shl(this->adaptReg(asmjit::x86::r10, rax_sz.size()), rax_sz);
          break;
        }
        case IRBinOp::Operation::SHR : {
          this->assembler->shr(this->adaptReg(asmjit::x86::r10, rax_sz.size()), rax_sz);
          break;
        }
      }
      asmjit::x86::Gp dest_sz = this->adaptReg(dest, rax_sz.size());
      if (dest.r64() != asmjit::x86::rax) {
        this->assembler->mov(dest_sz, rax_sz);
      }
      return dest_sz;
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
  this->assembler->mov(dest, value);
  return dest;
}