#include <wind/generation/IR.h>
#include <wind/backend/writer/writer.h>
#include <wind/backend/x86_64/backend.h>
#include <wind/bridge/opt_flags.h>
#include <stdexcept>
#include <iostream>

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

void WindEmitter::TryCast(Reg dst, Reg proc) {
    if (dst.size > proc.size) {
        this->writer->movzx(dst, proc);
    }
}

#define LITERAL_OP(type, op) \
    case type: { \
        this->writer->op(dst, binop->right()->as<IRLiteral>()->get()); \
        return; \
    }

#define LOCAL_OP_RAW(op) \
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
            LOCAL_OP_RAW(op) \
            return; \
        } \
    }

#define GLOB_OP_RAW(op) \
    this->writer->op(dst, this->writer->ptr( \
        binop->right()->as<IRGlobRef>()->getName(), \
        0, \
        binop->right()->as<IRGlobRef>()->getType()->moveSize() \
    )); \
    return;

#define GLOBAL_OP(type, op) \
    case type: { \
        if (freg) { \
            this->writer->op(dst, *freg); \
            return; \
        } else { \
            GLOB_OP_RAW(op) \
            return; \
        } \
    }

#define L_ASSIGN_OP() \
    this->writer->mov( \
        this->writer->ptr( \
            x86::Gp::rbp, \
            -binop->left()->as<IRLocalRef>()->offset(), \
            binop->left()->as<IRLocalRef>()->datatype()->moveSize() \
        ), \
        tmp \
    ); \

#define G_ASSIGN_OP() \
    this->writer->mov( \
        this->writer->ptr( \
            binop->left()->as<IRGlobRef>()->getName(), \
            0, \
            binop->left()->as<IRGlobRef>()->getType()->moveSize() \
        ), \
        tmp \
    ); \

#define LIT_CMP_OP(type, op) \
    case type: { \
        this->writer->cmp(dst, binop->right()->as<IRLiteral>()->get()); \
        if (!isJmp) { \
            this->writer->op(this->CastReg(dst, 1)); \
            this->TryCast(dst, x86::Gp::al); \
        } \
        return; \
    }

#define LOC_CMP_OP(type, op) \
    case type: { \
        if (freg) { \
            this->writer->cmp(dst, *freg); \
            return; \
        } else { \
            LOCAL_OP_RAW(cmp) \
            return; \
        } \
        if (!isJmp) { \
            this->writer->op(this->CastReg(dst, 1)); \
            this->TryCast(dst, x86::Gp::al); \
        } \
        return; \
    }

#define GLB_CMP_OP(type, op) \
    case type: { \
        this->writer->cmp(dst, this->writer->ptr( \
            binop->right()->as<IRGlobRef>()->getName(), \
            0, \
            binop->right()->as<IRGlobRef>()->getType()->moveSize() \
        )); \
        if (!isJmp) { \
            this->writer->op(this->CastReg(dst, 1)); \
            this->TryCast(dst, x86::Gp::al); \
        } \
        return; \
    }

#define EL_CMP_OP(op) \
    this->writer->cmp(dst, tmp); \
    if (!isJmp) { \
        this->writer->op(this->CastReg(dst, 1)); \
        this->TryCast(dst, x86::Gp::al); \
    } \


void WindEmitter::EmitBinOp(IRBinOp *binop, Reg dst, bool isJmp) {
    uint8_t tmp_size = dst.size;
    if (binop->operation() == IRBinOp::Operation::L_ASSIGN) {
        tmp_size = binop->left()->as<IRLocalRef>()->datatype()->moveSize();
    } else if (binop->operation() == IRBinOp::Operation::G_ASSIGN) {
        tmp_size = binop->left()->as<IRGlobRef>()->getType()->moveSize();
    }
    else {
        this->EmitExpr((IRNode*)binop->left(), dst);
        this->regalloc.SetDirty(dst);
    }

    switch (binop->right()->type()) {
        case IRNode::NodeType::LITERAL: {
            switch (binop->operation()) {
                LITERAL_OP(IRBinOp::ADD, add)
                LITERAL_OP(IRBinOp::SUB, sub)
                LITERAL_OP(IRBinOp::SHL, shl)
                LITERAL_OP(IRBinOp::SHR, shr)
                LITERAL_OP(IRBinOp::MUL, imul)
                LITERAL_OP(IRBinOp::AND, and_)
                LIT_CMP_OP(IRBinOp::EQ, sete)
                LIT_CMP_OP(IRBinOp::LESS, setl)
                LIT_CMP_OP(IRBinOp::GREATER, setg)
                LIT_CMP_OP(IRBinOp::LESSEQ, setle)
                default: {}
            }
        }
        case IRNode::NodeType::LOCAL_REF: {
            Reg *freg = this->regalloc.FindLocalVar(binop->right()->as<IRLocalRef>()->offset(), binop->right()->as<IRLocalRef>()->datatype()->moveSize());
            dst = this->CastReg(dst, binop->right()->as<IRLocalRef>()->datatype()->moveSize());
            switch (binop->operation()) {
                OPT_LOCAL_OP(IRBinOp::ADD, add)
                OPT_LOCAL_OP(IRBinOp::SUB, sub)
                OPT_LOCAL_OP(IRBinOp::SHL, shl)
                OPT_LOCAL_OP(IRBinOp::SHR, shr)
                OPT_LOCAL_OP(IRBinOp::MUL, imul)
                OPT_LOCAL_OP(IRBinOp::AND, and_)
                LOC_CMP_OP(IRBinOp::EQ, sete)
                LOC_CMP_OP(IRBinOp::LESS, setl)
                LOC_CMP_OP(IRBinOp::GREATER, setg)
                LOC_CMP_OP(IRBinOp::LESSEQ, setle)
                default: {}
            }
        }
        case IRNode::NodeType::GLOBAL_REF: {
            Reg *freg = this->regalloc.FindLabel(binop->right()->as<IRGlobRef>()->getName(), binop->right()->as<IRGlobRef>()->getType()->moveSize());
            dst = this->CastReg(dst, binop->right()->as<IRGlobRef>()->getType()->moveSize());
            switch (binop->operation()) {
                GLOBAL_OP(IRBinOp::ADD, add)
                GLOBAL_OP(IRBinOp::SUB, sub)
                GLOBAL_OP(IRBinOp::SHL, shl)
                GLOBAL_OP(IRBinOp::SHR, shr)
                GLOBAL_OP(IRBinOp::MUL, imul)
                GLOBAL_OP(IRBinOp::AND, and_)
                GLB_CMP_OP(IRBinOp::EQ, sete)
                GLB_CMP_OP(IRBinOp::LESS, setl)
                GLB_CMP_OP(IRBinOp::GREATER, setg)
                GLB_CMP_OP(IRBinOp::LESSEQ, setle)
                default: {}
            }
        }
        default: {}
    }

    Reg tmp = this->regalloc.Allocate(tmp_size, false);
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
        case IRBinOp::AND:
            this->writer->and_(dst, tmp);
            break;
        case IRBinOp::L_ASSIGN:
            L_ASSIGN_OP()
            break;
        case IRBinOp::G_ASSIGN:
            G_ASSIGN_OP()
            break;
        case IRBinOp::EQ:
            EL_CMP_OP(sete)
            break;
        case IRBinOp::LESS:
            EL_CMP_OP(setl)
            break;
        case IRBinOp::GREATER:
            EL_CMP_OP(setg)
            break;
        case IRBinOp::LESSEQ:
            EL_CMP_OP(setle)
            break;
        default:
            throw std::runtime_error("Unsupported binop (" + std::to_string((uint8_t)binop->operation()) + "): Report to mantainer!");
    }

    this->regalloc.Free(tmp);
    if (binop->operation() == IRBinOp::Operation::L_ASSIGN) {
        this->regalloc.SetVar(dst, binop->left()->as<IRLocalRef>()->offset(), RegisterAllocator::RegValue::Lifetime::UNTIL_ALLOC);
    } else if (binop->operation() == IRBinOp::Operation::G_ASSIGN) {
        this->regalloc.SetLabel(dst, binop->left()->as<IRGlobRef>()->getName(), RegisterAllocator::RegValue::Lifetime::UNTIL_ALLOC);
    }
}

void WindEmitter::EmitExpr(IRNode *value, Reg dst, bool isJmp) {
    if (!this->regalloc.Request(dst)) {
        std::cerr << this->GetAsm() << std::endl;
        this->regalloc.AllocRepr();
        throw std::runtime_error("Register allocation failed");
    }

    bool jmpProcessed=false;
    switch (value->type()) {
        case IRNode::NodeType::BIN_OP: {
            this->EmitBinOp(value->as<IRBinOp>(), dst, false);
            jmpProcessed=true;
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