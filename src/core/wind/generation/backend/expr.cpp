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

void WindEmitter::moveIfNot(asmjit::x86::Gp dest, asmjit::x86::Gp src) {
  if (dest != src) {
    this->assembler->mov(dest, src);
  }
}

asmjit::x86::Gp WindEmitter::emitBinOp(IRBinOp *bin_op, asmjit::x86::Gp dest) {
  IRNode *right_node = (IRNode*)bin_op->right();
  asmjit::x86::Gp left;

  if (bin_op->operation() == IRBinOp::Operation::ASSIGN) {
    if (auto left_ref = ((IRNode*)bin_op->left())->as<IRLocalRef>()) {
      moveIntoVar(left_ref, right_node);
      return dest;
    } else if (auto left_addr = ((IRNode*)bin_op->left())->as<IRLocalAddrRef>();
              left_addr && left_addr->isIndexed()) {
      moveIntoIndex(left_addr, right_node);
      return dest;
    }
  }

  if (auto right_bin_op = dynamic_cast<IRBinOp*>(right_node)) {
    left = emitExpr((IRNode*)bin_op->left(), asmjit::x86::rdx);
    right_node = new IRRegister(emitBinOp(right_bin_op, asmjit::x86::rax));
  } else {
    left = emitExpr((IRNode*)bin_op->left(), asmjit::x86::rax);
  }

  // Helper to handle operations with literals
  auto handleLiteralOp = [&](IRLiteral* lit) {
    switch (bin_op->operation()) {
      case IRBinOp::Operation::ADD: assembler->add(left, lit->get()); break;
      case IRBinOp::Operation::SUB: assembler->sub(left, lit->get()); break;
      case IRBinOp::Operation::MUL: assembler->imul(left, lit->get()); break;
      case IRBinOp::Operation::DIV: throw std::runtime_error("TODO: Implement division");
      case IRBinOp::Operation::MOD: {
        this->assembler->mov(asmjit::x86::rdx, 0);
        this->moveIfNot(asmjit::x86::rax, left);
        this->assembler->mov(asmjit::x86::rcx, lit->get());
        this->assembler->div(asmjit::x86::rcx);
        this->moveIfNot(dest, asmjit::x86::rdx);
        break;
      }

      case IRBinOp::Operation::SHL: assembler->shl(left, lit->get()); break;
      case IRBinOp::Operation::SHR: assembler->shr(left, lit->get()); break;


      case IRBinOp::Operation::EQ:  {
        this->assembler->cmp(left, lit->get());
        this->assembler->sete(dest.r8());
        left = dest.r8();
        break;
      }
      case IRBinOp::Operation::LESS: {
        this->assembler->cmp(left, lit->get());
        this->assembler->setl(dest.r8());
        left = dest.r8();
        break;
      }

      case IRBinOp::Operation::LOGAND : {
        this->assembler->test(left, lit->get());
        this->assembler->setne(dest.r8());
        left = dest.r8();
        break;
      }

      default: throw std::runtime_error("Unknown operation");
    }
    if (left != dest) assembler->mov(dest, left);
    OptClearReg(left);
    return left;
  };

  auto handleRegisterOp = [&](asmjit::x86::Gp right) {
    switch (bin_op->operation()) {
        case IRBinOp::Operation::ADD: assembler->add(right, left); break;
        case IRBinOp::Operation::SUB: assembler->sub(right, left); break;
        case IRBinOp::Operation::MUL: assembler->imul(right, left); break;
        case IRBinOp::Operation::DIV: throw std::runtime_error("TODO: Implement division");
        case IRBinOp::Operation::MOD: {
          this->assembler->mov(asmjit::x86::rdx, 0);
          this->moveIfNot(asmjit::x86::rax, left);
          this->assembler->div(right);
          this->moveIfNot(dest, asmjit::x86::rdx);
          break;
        }

        case IRBinOp::Operation::SHL: assembler->shl(right, left); break;
        case IRBinOp::Operation::SHR: assembler->shr(right, left); break;

        case IRBinOp::Operation::ASSIGN: assembler->mov(right, left); break;

        case IRBinOp::Operation::EQ: {
          this->assembler->cmp(left, right);
          this->assembler->sete(dest.r8());
          right = dest.r8();
          break;
        }
        case IRBinOp::Operation::LESS: {
          this->assembler->cmp(left, right);
          this->assembler->setl(dest.r8());
          right = dest.r8();
          break;
        }

        case IRBinOp::Operation::LOGAND: {
          this->assembler->test(left, right);
          this->assembler->setne(dest.r8());
          right = dest.r8();
          break;
        }

        default: throw std::runtime_error("Unknown operation");
    }
    if (right != dest) assembler->mov(dest, right);
    OptClearReg(right);
    return right;
  };

  switch (right_node->type()) {
    case IRNode::NodeType::LITERAL:
        return handleLiteralOp(dynamic_cast<IRLiteral*>(right_node));

    case IRNode::NodeType::FUNCTION_CALL: {
        assembler->mov(asmjit::x86::rdx, left);
        asmjit::x86::Gp rax_sz = emitExpr(right_node, asmjit::x86::rax);
        asmjit::x86::Gp dest_sz = adaptReg(dest, rax_sz.size());
        handleRegisterOp(adaptReg(asmjit::x86::rdx, rax_sz.size()));
        if (dest.r64() != asmjit::x86::rax) assembler->mov(dest_sz, rax_sz);
        OptClearReg(dest_sz);
        return dest_sz;
    }

    case IRNode::NodeType::LOCAL_REF: {
      auto local = dynamic_cast<IRLocalRef*>(right_node);
      if (!local) throw std::runtime_error("Invalid local reference");
      if (local->datatype()->isArray()) throw std::runtime_error("TODO: Implement array binop");
      left = adaptReg(left, local->datatype()->moveSize());
      auto local_ptr = asmjit::x86::ptr(asmjit::x86::rbp, -local->offset(), local->datatype()->moveSize());
      switch (bin_op->operation()) {
        case IRBinOp::Operation::ADD: assembler->add(left, local_ptr); break;
        case IRBinOp::Operation::SUB: assembler->sub(left, local_ptr); break;
        case IRBinOp::Operation::MUL: assembler->imul(left, local_ptr); break;
        case IRBinOp::Operation::DIV: throw std::runtime_error("TODO: Implement division");
        case IRBinOp::Operation::MOD: {
          this->assembler->mov(asmjit::x86::rdx, 0);
          this->moveIfNot(asmjit::x86::rax, left);
          this->assembler->div(local_ptr);
          this->moveIfNot(dest, asmjit::x86::rdx);
          break;
        }

        case IRBinOp::Operation::SHL: assembler->shl(left, moveVar(local, asmjit::x86::rdx)); break;
        case IRBinOp::Operation::SHR: assembler->shr(left, moveVar(local, asmjit::x86::rdx)); break;

        case IRBinOp::Operation::EQ : {
          this->assembler->cmp(left, local_ptr);
          this->assembler->sete(dest.r8());
          left = dest.r8();
          break;
        }
        case IRBinOp::Operation::LESS: {
          this->assembler->cmp(left, local_ptr);
          this->assembler->setl(dest.r8());
          left = dest.r8();
          break;
        }

        case IRBinOp::Operation::LOGAND: {
          asmjit::x86::Gp vreg = this->adaptReg(asmjit::x86::rdx, local->datatype()->moveSize());
          this->assembler->mov(vreg, local_ptr);
          this->assembler->test(left, vreg);
          this->assembler->setne(dest.r8());
          left = dest.r8();
          break;
        }

        default: throw std::runtime_error("Unknown operation");
      }
      if (left != dest) assembler->mov(dest, left);
      OptClearReg(left);
      return left;
    }

    case IRNode::NodeType::REGISTER:
      return handleRegisterOp(dynamic_cast<IRRegister*>(right_node)->reg());

    default:
      throw std::runtime_error("Unknown node type");
    }
  }


  asmjit::x86::Gp WindEmitter::emitLiteral(IRLiteral *lit, asmjit::x86::Gp dest) {
    long long value = lit->get();
    this->assembler->mov(dest, value);
    this->OptClearReg(dest);
    this->OptClearReg(dest);
    return dest;
  }