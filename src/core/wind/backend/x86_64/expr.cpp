#include <wind/generation/IR.h>
#include <wind/backend/writer/writer.h>
#include <wind/backend/x86_64/backend.h>
#include <wind/bridge/opt_flags.h>
#include <stdexcept>
#include <iostream>

void WindEmitter::EmitLocRef(IRLocalRef *ref, Reg dst) {
    Reg *freg = this->regalloc.FindLocalVar(ref->offset());
    if (freg) {
        if (freg->id != dst.id) {
            this->writer->mov(dst, *freg);
        }
        return;
    }
    this->writer->mov(
        dst,
        this->writer->ptr(
            x86::Gp::rbp,
            -ref->offset(),
            ref->datatype()->moveSize()
        )
    );
    this->regalloc.SetVar(dst, ref->offset(), RegisterAllocator::RegValue::Lifetime::UNTIL_ALLOC);
}

void WindEmitter::EmitValue(IRNode *value, Reg dst) {
    switch (value->type()) {
        case IRNode::NodeType::LITERAL: {
            this->writer->mov(dst, value->as<IRLiteral>()->get());
            break;
        }
        case IRNode::NodeType::LOCAL_REF: {
            this->EmitLocRef(value->as<IRLocalRef>(), dst);
            break;
        }
        case IRNode::NodeType::GLOBAL_REF: {
            this->EmitGlobRef(value->as<IRGlobRef>(), dst);
            break;
        }
        default: {
            throw std::runtime_error("Unknown value type(" + std::to_string((uint8_t)value->type()) + "): Report to mantainer!");
        }
    }
}

#define LITERAL_OP(type, op) \
    case type: { \
        this->writer->op(dst, binop->right()->as<IRLiteral>()->get()); \
        return; \
    }

#define LOCAL_OP(type, op) \
    case type: { \
        this->writer->op(dst, this->writer->ptr( \
            x86::Gp::rbp, \
            -binop->right()->as<IRLocalRef>()->offset(), \
            binop->right()->as<IRLocalRef>()->datatype()->moveSize() \
        )); \
        return; \
    }

#define GLOBAL_OP(type, op) \
    case type: { \
        this->writer->op(dst, this->writer->ptr( \
            binop->right()->as<IRGlobRef>()->getName(), \
            0, \
            binop->right()->as<IRGlobRef>()->getType()->moveSize() \
        )); \
        return; \
    }

void WindEmitter::EmitBinOp(IRBinOp *binop, Reg dst) {
    this->EmitExpr((IRNode*)binop->left(), dst);
    this->regalloc.SetDirty(dst);

    switch (binop->right()->type()) {
        case IRNode::NodeType::LITERAL: {
            switch (binop->operation()) {
                LITERAL_OP(IRBinOp::ADD, add)
                LITERAL_OP(IRBinOp::SUB, sub)
                LITERAL_OP(IRBinOp::SHL, shl)
                LITERAL_OP(IRBinOp::SHR, shr)
            }
        }
        case IRNode::NodeType::LOCAL_REF: {
            switch (binop->operation()) {
                LOCAL_OP(IRBinOp::ADD, add)
                LOCAL_OP(IRBinOp::SUB, sub)
                LOCAL_OP(IRBinOp::SHL, shl)
                LOCAL_OP(IRBinOp::SHR, shr)
            }
        }
        case IRNode::NodeType::GLOBAL_REF: {
            switch (binop->operation()) {
                GLOBAL_OP(IRBinOp::ADD, add)
                GLOBAL_OP(IRBinOp::SUB, sub)
                GLOBAL_OP(IRBinOp::SHL, shl)
                GLOBAL_OP(IRBinOp::SHR, shr)
            }
        }
    }

    Reg tmp = this->regalloc.Allocate(dst.size, false);
    this->EmitExpr((IRNode*)binop->right(), tmp);
    this->regalloc.SetDirty(tmp);

    switch (binop->operation()) {
        case IRBinOp::ADD:
            this->writer->add(dst, tmp);
            break;
        case IRBinOp::SUB:
            this->writer->sub(dst, tmp);
            break;
        case IRBinOp::SHL:
            this->writer->shl(dst, tmp);
            break;
        case IRBinOp::SHR:
            this->writer->shr(dst, tmp);
            break;
        default:
            throw std::runtime_error("Unsupported binop (" + std::to_string((uint8_t)binop->operation()) + "): Report to mantainer!");
    }

    this->regalloc.Free(tmp);
}

void WindEmitter::EmitExpr(IRNode *value, Reg dst) {
    if (!this->regalloc.Request(dst)) {
        throw std::runtime_error("Register allocation failed");
    }

    switch (value->type()) {
        case IRNode::NodeType::BIN_OP: {
            this->EmitBinOp(value->as<IRBinOp>(), dst);
            break;
        }
        default: {
            this->EmitValue(value, dst);
        }
    }

    this->regalloc.PostExpression();
}