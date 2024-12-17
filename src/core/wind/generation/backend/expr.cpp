#include <wind/generation/backend.h>
#include <wind/common/debug.h>
#include <asmjit/asmjit.h>
#include <iostream>

asmjit::x86::Gp WindEmitter::moveVar(IRLocalRef *local, asmjit::x86::Gp dest) {
  asmjit::x86::Gp reg = adaptReg(dest, local->datatype()->moveSize());
  asmjit::x86::Gp var_reg = OptVarGet(local->offset());
  if (this->OptVarGet(local->offset()) == reg) {
    return reg;
  } else if (var_reg != asmjit::x86::r15) {
    this->assembler->mov(reg, var_reg);
    return reg;
  }
  if (local->datatype()->isArray()) {
    // Arrays turn into pointers
    this->emitLRef(new IRLocalAddrRef(local->offset(), local->datatype()), reg);
    return reg;
  }
  this->assembler->mov(reg, asmjit::x86::ptr(asmjit::x86::rbp, -local->offset(), local->datatype()->moveSize()));
  this->OptVarMoved(local->offset(), reg);
  return reg;
}

void WindEmitter::LitIntoVar(IRLocalRef *local, IRLiteral *lit) {
  if (local->datatype()->isArray()) {
    std::runtime_error("TODO: Move into array");
  }
  this->assembler->mov(
  asmjit::x86::ptr(asmjit::x86::rbp, -local->offset(), local->datatype()->moveSize()),
  lit->get()
);
}

void WindEmitter::moveIntoVar(IRLocalRef *local, IRNode *value) {
  if (local->datatype()->isArray()) {
    std::runtime_error("TODO: Move into array");
  }
  if (value->is<IRLiteral>()) {
    IRLiteral *lit = value->as<IRLiteral>();
    this->LitIntoVar(local, lit);
  } else {
    asmjit::x86::Gp reg = this->adaptReg(asmjit::x86::rax, local->datatype()->moveSize());
    asmjit::x86::Gp src = this->adaptReg(this->emitExpr(value, reg), local->datatype()->moveSize());
    this->assembler->mov(asmjit::x86::ptr(asmjit::x86::rbp, -local->offset(), local->datatype()->moveSize()), src);
  }
}

void WindEmitter::moveIntoIndex(const IRLocalAddrRef *local, IRNode *value) {
  if (local->datatype()->isArray()) {
    uint16_t offset = local->offset() - local->datatype()->index2offset(local->getIndex());
    this->moveIntoVar(new IRLocalRef(offset, local->datatype()->getArrayType()), value);
  } else {
    if (local->datatype()->rawSize() == DataType::Sizes::QWORD) {
      // Assume 64bit address
      // Retrieve pointing type from value
      this->assembler->mov(
        asmjit::x86::rbx,
        asmjit::x86::ptr(asmjit::x86::rbp, -local->offset(), 8)
      );
      asmjit::x86::Gp res = this->emitExpr(value, asmjit::x86::rax);
      this->assembler->mov(
        asmjit::x86::ptr(asmjit::x86::rbx, -local->getIndex()*res.size(), res.size()),
        res
      );
    }
  }
}

asmjit::x86::Gp WindEmitter::emitBinOp(IRBinOp *bin_op, asmjit::x86::Gp dest) {
  IRNode *right_node = (IRNode*)bin_op->right();
  asmjit::x86::Gp left;
  if (bin_op->operation() == IRBinOp::Operation::ASSIGN) {
    if (bin_op->left()->is<IRLocalRef>()) {
      this->moveIntoVar((IRLocalRef*)bin_op->left()->as<IRLocalRef>(), right_node);
      return dest;
    } else if (bin_op->left()->is<IRLocalAddrRef>() && bin_op->left()->as<IRLocalAddrRef>()->isIndexed()) {
      this->moveIntoIndex(bin_op->left()->as<IRLocalAddrRef>(), right_node);
      return dest;
    }
  }
  else if (right_node->is<IRBinOp>()) {
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
        default: {
          std::cout << "Unknown operation" << std::endl;
          break;
        }
      }
      if (left != dest) {
        this->assembler->mov(dest, left);
      }
      this->OptClearReg(left);
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
        default: {
          std::cout << "Unknown operation" << std::endl;
          break;
        }
      }
      asmjit::x86::Gp dest_sz = this->adaptReg(dest, rax_sz.size());
      if (dest.r64() != asmjit::x86::rax) {
        this->assembler->mov(dest_sz, rax_sz);
      }
      this->OptClearReg(dest_sz);
      return dest_sz;
    }

    case IRNode::NodeType::LOCAL_REF : {
      IRLocalRef *local = right_node->as<IRLocalRef>();
      if (local->datatype()->isArray()) {
        std::runtime_error("TODO: Implement array binop");
      }
      left = this->adaptReg(left, local->datatype()->moveSize());
      switch (bin_op->operation()) {
        case IRBinOp::Operation::ADD : {
          this->assembler->add(left, asmjit::x86::ptr(asmjit::x86::rbp, -local->offset(), local->datatype()->moveSize()));
          break;
        }
        case IRBinOp::Operation::SUB : {
          this->assembler->sub(left, asmjit::x86::ptr(asmjit::x86::rbp, -local->offset(), local->datatype()->moveSize()));
          break;
        }
        case IRBinOp::Operation::MUL : {
          this->assembler->imul(left, asmjit::x86::ptr(asmjit::x86::rbp, -local->offset(), local->datatype()->moveSize()));
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
        default: {
          std::cout << "Unknown operation" << std::endl;
          break;
        }
      }
      if (left != dest) {
        this->assembler->mov(dest, left);
      }
      this->OptClearReg(left);
      return left;
    }

    case IRNode::NodeType::REGISTER : {
      asmjit::x86::Gp right = right_node->as<IRRegister>()->reg();
      switch (bin_op->operation()) {
        case IRBinOp::Operation::ADD : {
          this->assembler->add(right, left);
          break;
        }
        case IRBinOp::Operation::SUB : {
          this->assembler->sub(right, left);
          break;
        }
        case IRBinOp::Operation::MUL : {
          this->assembler->imul(right, left);
          break;
        }
        case IRBinOp::Operation::DIV : {
          throw std::runtime_error("TODO: Implement division");
          break;
        }
        case IRBinOp::Operation::SHL : {
          this->assembler->shl(right, left);
          break;
        }
        case IRBinOp::Operation::SHR : {
          this->assembler->shr(right, left);
          break;
        }
        case IRBinOp::Operation::ASSIGN : {
          this->assembler->mov(right, left);
          break;
        }
        default : {
          std::cout << "Unknown operation" << std::endl;
          break;
        }
      }
      if (right != dest) {
        this->assembler->mov(dest, right);
      }
      this->OptClearReg(right);
      return right;
    }

    default: {
      std::cout << "Unknown node type" << std::endl;
      break;
    }
  }
  this->OptClearReg(dest);
  return dest;
}

asmjit::x86::Gp WindEmitter::emitLiteral(IRLiteral *lit, asmjit::x86::Gp dest) {
  long long value = lit->get();
  this->assembler->mov(dest, value);
  this->OptClearReg(dest);
  this->OptClearReg(dest);
  return dest;
}