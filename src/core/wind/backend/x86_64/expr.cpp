#include <wind/generation/IR.h>
#include <wind/backend/writer/writer.h>
#include <wind/backend/x86_64/backend.h>
#include <wind/bridge/opt_flags.h>
#include <stdexcept>
#include <iostream>

void WindEmitter::EmitLocRef(IRLocalRef *ref, Reg dst) {
    Reg *freg = this->regalloc.FindLocalVar(ref->offset(), ref->datatype()->moveSize());
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

void WindEmitter::EmitString(IRStringLiteral *str, Reg dst) {
    this->rostrs.push_back(str->get());
    this->writer->lea(
        dst,
        this->writer->ptr(
            ".ros"+std::to_string(this->rostr_i++),
            0,
            8
        )
    );
}

void WindEmitter::EmitLocAddrRef(IRLocalAddrRef *ref, Reg dst) {
    if (ref->isIndexed()) {
        if (!ref->datatype()->isArray()) {
            throw std::runtime_error("Cannot index non-array");
        }
        uint16_t offset = ref->offset() - ref->datatype()->index2offset(ref->getIndex());
        if (offset < 0) {
            throw std::runtime_error("Invalid offset");
        }
        this->writer->mov(
            dst,
            this->writer->ptr(
                x86::Gp::rbp,
                -offset,
                ref->datatype()->rawSize()
            )
        );
    } else {
        this->writer->lea(
            dst,
            this->writer->ptr(
                x86::Gp::rbp,
                -ref->offset(),
                8
            )
        );
    }
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
        case IRNode::NodeType::STRING: {
            this->EmitString(value->as<IRStringLiteral>(), dst);
            break;
        }
        case IRNode::NodeType::LADDR_REF: {
            this->EmitLocAddrRef(value->as<IRLocalAddrRef>(), dst);
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

#define LOCAL_OP_RAW(type, op) \
    this->writer->op(dst, this->writer->ptr( \
        x86::Gp::rbp, \
        -binop->right()->as<IRLocalRef>()->offset(), \
        binop->right()->as<IRLocalRef>()->datatype()->moveSize() \
    )); \
    return;

#define OPT_LOCAL_OP(type, op) \
    case type: { \
        if (freg) { \
            this->writer->op(dst, *freg); \
            return; \
        } else { \
            LOCAL_OP_RAW(type, op) \
            return; \
        } \
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
                LITERAL_OP(IRBinOp::MUL, imul)
                default: {}
            }
        }
        case IRNode::NodeType::LOCAL_REF: {
            Reg *freg = this->regalloc.FindLocalVar(binop->right()->as<IRLocalRef>()->offset(), binop->right()->as<IRLocalRef>()->datatype()->moveSize());
                switch (binop->operation()) {
                    OPT_LOCAL_OP(IRBinOp::ADD, add)
                    OPT_LOCAL_OP(IRBinOp::SUB, sub)
                    OPT_LOCAL_OP(IRBinOp::SHL, shl)
                    OPT_LOCAL_OP(IRBinOp::SHR, shr)
                    OPT_LOCAL_OP(IRBinOp::MUL, imul)
                    default: {}
                }
        }
        case IRNode::NodeType::GLOBAL_REF: {
            switch (binop->operation()) {
                GLOBAL_OP(IRBinOp::ADD, add)
                GLOBAL_OP(IRBinOp::SUB, sub)
                GLOBAL_OP(IRBinOp::SHL, shl)
                GLOBAL_OP(IRBinOp::SHR, shr)
                GLOBAL_OP(IRBinOp::MUL, imul)
                default: {}
            }
        }
        default: {}
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
        case IRBinOp::MUL:
            this->writer->imul(dst, tmp);
            break;
        default:
            throw std::runtime_error("Unsupported binop (" + std::to_string((uint8_t)binop->operation()) + "): Report to mantainer!");
    }

    this->regalloc.Free(tmp);
}

void WindEmitter::EmitExpr(IRNode *value, Reg dst) {
    if (!this->regalloc.Request(dst)) {
        std::cerr << this->GetAsm() << std::endl;
        this->regalloc.AllocRepr();
        throw std::runtime_error("Register allocation failed");
    }

    switch (value->type()) {
        case IRNode::NodeType::BIN_OP: {
            this->EmitBinOp(value->as<IRBinOp>(), dst);
            break;
        }
        case IRNode::NodeType::FUNCTION_CALL: {
            this->EmitFnCall(value->as<IRFnCall>(), dst);
            break;
        }
        default: {
            this->EmitValue(value, dst);
        }
    }

    this->regalloc.PostExpression();
}