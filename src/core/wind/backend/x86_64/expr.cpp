#include <wind/generation/IR.h>
#include <wind/backend/writer/writer.h>
#include <wind/backend/x86_64/backend.h>
#include <wind/bridge/opt_flags.h>
#include <stdexcept>
#include <iostream>

Reg WindEmitter::EmitString(IRStringLiteral *str, Reg dst) {
    this->rostrs.push_back(str->get());
    this->writer->lea(
        dst,
        this->writer->ptr(
            ".ros"+std::to_string(this->rostr_i++),
            0,
            8
        )
    );
    return {dst.id, 8, Reg::GPR, false};
}

Reg WindEmitter::EmitLocAddrRef(IRLocalAddrRef *ref, Reg dst) {
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
    return {dst.id, 8, Reg::GPR, false};
}

Reg WindEmitter::EmitValue(IRNode *value, Reg dst) {
    switch (value->type()) {
        case IRNode::NodeType::LITERAL: {
            int64_t val = value->as<IRLiteral>()->get();
            if (val==0) {
                this->writer->xor_(dst, dst);
            } else {
                this->writer->mov(dst, val);
            }
            break;
        }
        case IRNode::NodeType::LOCAL_REF: {
            return this->EmitLocRef(value->as<IRLocalRef>(), dst);
        }
        case IRNode::NodeType::GLOBAL_REF: {
            return this->EmitGlobRef(value->as<IRGlobRef>(), dst);
        }
        case IRNode::NodeType::STRING: {
            return this->EmitString(value->as<IRStringLiteral>(), dst);
        }
        case IRNode::NodeType::LADDR_REF: {
            return this->EmitLocAddrRef(value->as<IRLocalAddrRef>(), dst);
        }
        default: {
            throw std::runtime_error("Unknown value type(" + std::to_string((uint8_t)value->type()) + "): Report to maintainer!");
        }
    }
    return dst;
}

void WindEmitter::TryCast(Reg dst, Reg proc) {
    if (dst.size > proc.size) {
        if (!dst.signed_value) {
            this->writer->movzx(dst, proc);
        } else {
            this->writer->movsx(dst, proc);
        }
    } else if (dst.id != proc.id) {
        this->writer->mov(dst, proc);
    }
}

#define LITERAL_OP(type, op) \
    case type: { \
        this->writer->op(dst, binop->right()->as<IRLiteral>()->get()); \
        return dst; \
    }

#define LOCAL_OP_RAW(op) \
    this->writer->op(dst, this->writer->ptr( \
        x86::Gp::rbp, \
        -binop->right()->as<IRLocalRef>()->offset(), \
        binop->right()->as<IRLocalRef>()->datatype()->moveSize() \
    )); \
    return {dst.id, (uint8_t)binop->right()->as<IRLocalRef>()->datatype()->moveSize(), Reg::GPR, binop->right()->as<IRLocalRef>()->datatype()->isSigned()};

#define OPT_LOCAL_OP(type, op) \
    case type: { \
        if (freg) { \
            this->writer->op(dst, *freg); \
            dst.signed_value = binop->right()->as<IRLocalRef>()->datatype()->isSigned(); \
            return dst; \
        } else { \
            LOCAL_OP_RAW(op) \
        } \
    }

#define GLOB_OP_RAW(op) \
    this->writer->op(dst, this->writer->ptr( \
        binop->right()->as<IRGlobRef>()->getName(), \
        0, \
        binop->right()->as<IRGlobRef>()->getType()->moveSize() \
    )); \
    return {dst.id, (uint8_t)binop->right()->as<IRGlobRef>()->getType()->moveSize(), Reg::GPR, binop->right()->as<IRGlobRef>()->getType()->isSigned()};

#define GLOBAL_OP(type, op) \
    case type: { \
        if (freg) { \
            this->writer->op(dst, *freg); \
            return dst; \
        } else { \
            GLOB_OP_RAW(op) \
        } \
    }

#define L_ASSIGN_OP(src) \
    this->writer->mov( \
        this->writer->ptr( \
            x86::Gp::rbp, \
            -binop->left()->as<IRLocalRef>()->offset(), \
            binop->left()->as<IRLocalRef>()->datatype()->moveSize() \
        ), \
        src \
    ); \

#define G_ASSIGN_OP(src) \
    this->writer->mov( \
        this->writer->ptr( \
            binop->left()->as<IRGlobRef>()->getName(), \
            0, \
            binop->left()->as<IRGlobRef>()->getType()->moveSize() \
        ), \
        src \
    ); \

#define L_PLUS_ASSIGN_OP(src) \
    this->writer->add( \
        this->writer->ptr( \
            x86::Gp::rbp, \
            -binop->left()->as<IRLocalRef>()->offset(), \
            binop->left()->as<IRLocalRef>()->datatype()->moveSize() \
        ), \
        src \
    ); \

#define G_PLUS_ASSIGN_OP(src) \
    this->writer->add( \
        this->writer->ptr( \
            binop->left()->as<IRGlobRef>()->getName(), \
            0, \
            binop->left()->as<IRGlobRef>()->getType()->moveSize() \
        ), \
        src \
    ); \

#define L_MINUS_ASSIGN_OP(src) \
    this->writer->sub( \
        this->writer->ptr( \
            x86::Gp::rbp, \
            -binop->left()->as<IRLocalRef>()->offset(), \
            binop->left()->as<IRLocalRef>()->datatype()->moveSize() \
        ), \
        src \
    ); \

#define G_MINUS_ASSIGN_OP(src) \
    this->writer->sub( \
        this->writer->ptr( \
            binop->left()->as<IRGlobRef>()->getName(), \
            0, \
            binop->left()->as<IRGlobRef>()->getType()->moveSize() \
        ), \
        src \
    ); \

#define LIT_CMP_OP(type, op) \
    case type: { \
        this->writer->cmp(dst, binop->right()->as<IRLiteral>()->get()); \
        dst.size = tmp_size; \
        if (!isJmp) { \
            this->writer->op(this->CastReg(dst, 1)); \
            this->TryCast(dst, x86::Gp::al); \
        } \
        return dst; \
    }

#define LOC_CMP_OP(type, op) \
    case type: { \
        if (freg) { \
            this->writer->cmp(dst, *freg); \
            return dst; \
        } else { \
            LOCAL_OP_RAW(cmp) \
        } \
        dst.size = binop->right()->as<IRLocalRef>()->datatype()->moveSize(); \
        dst.signed_value = binop->right()->as<IRLocalRef>()->datatype()->isSigned(); \
        if (!isJmp) { \
            this->writer->op(this->CastReg(dst, 1)); \
            this->TryCast(dst, x86::Gp::al); \
        } \
        return dst; \
    }

#define GLB_CMP_OP(type, op) \
    case type: { \
        this->writer->cmp(dst, this->writer->ptr( \
            binop->right()->as<IRGlobRef>()->getName(), \
            0, \
            binop->right()->as<IRGlobRef>()->getType()->moveSize() \
        )); \
        dst.size = binop->right()->as<IRGlobRef>()->getType()->moveSize(); \
        dst.signed_value = binop->right()->as<IRGlobRef>()->getType()->isSigned(); \
        if (!isJmp) { \
            this->writer->op(this->CastReg(dst, 1)); \
            this->TryCast(dst, x86::Gp::al); \
        } \
        return dst; \
    }

#define EL_CMP_OP(op) \
    this->writer->cmp(dst, tmp); \
    if (!isJmp) { \
        this->writer->op(this->CastReg(dst, 1)); \
        this->TryCast(dst, x86::Gp::al); \
    } \

#define AFTER_LOC_ASSIGN() \
    dst.size = binop->left()->as<IRLocalRef>()->datatype()->moveSize(); \
    dst.signed_value = binop->left()->as<IRLocalRef>()->datatype()->isSigned(); \
    this->writer->mov(dst, this->writer->ptr( \
        x86::Gp::rbp, \
        -binop->left()->as<IRLocalRef>()->offset(), \
        binop->left()->as<IRLocalRef>()->datatype()->moveSize() \
    ));

#define AFTER_GLB_ASSIGN() \
    dst.size = binop->left()->as<IRGlobRef>()->getType()->moveSize(); \
    dst.signed_value = binop->left()->as<IRGlobRef>()->getType()->isSigned(); \
    this->writer->mov(dst, this->writer->ptr( \
        binop->left()->as<IRGlobRef>()->getName(), \
        0, \
        binop->left()->as<IRGlobRef>()->getType()->moveSize() \
    ));

#define GEN_ASSIGN_LIT(type, op) \
    case type: { \
        op(binop->right()->as<IRLiteral>()->get()); \
        if (binop->left()->is<IRLocalRef>()) { \
            AFTER_LOC_ASSIGN() \
            this->regalloc.SetVar(dst, binop->left()->as<IRLocalRef>()->offset(), RegisterAllocator::RegValue::Lifetime::UNTIL_ALLOC); \
        } \
        else {\
            AFTER_GLB_ASSIGN() \
            this->regalloc.SetLabel(dst, binop->left()->as<IRGlobRef>()->getName(), RegisterAllocator::RegValue::Lifetime::UNTIL_ALLOC); \
        } \
        return dst; \
    }

#define LIT_RAW_DIV(op) \
    this->TryCast(x86::Gp::rax, dst); \
    this->writer->xor_(x86::Gp::rdx, x86::Gp::rdx); \
    this->writer->mov(x86::Gp::r10, binop->right()->as<IRLiteral>()->get()); \
    this->writer->op(x86::Gp::r10); \

#define LIT_DIV(type, op) \
    case type: { \
        LIT_RAW_DIV(op) \
        this->TryCast(dst, x86::Gp::rax); \
        return dst; \
    }

#define LIT_MOD(type, op) \
    case type: { \
        LIT_RAW_DIV(op) \
        this->TryCast(dst, x86::Gp::rdx); \
        return dst; \
    }

#define LOC_RAW_DIV(op) \
    this->TryCast(x86::Gp::rax, dst); \
    this->writer->xor_(x86::Gp::rdx, x86::Gp::rdx); \
    if (freg) { \
        this->writer->op(*freg); \
    } else { \
        this->writer->mov(x86::Gp::r10, this->writer->ptr( \
            x86::Gp::rbp, \
            -binop->right()->as<IRLocalRef>()->offset(), \
            binop->right()->as<IRLocalRef>()->datatype()->moveSize() \
        )); \
        this->writer->op(x86::Gp::r10); \
    } \

#define LOCAL_DIV(type, op) \
    case type: { \
        LOC_RAW_DIV(op) \
        this->TryCast(dst, x86::Gp::rax); \
        return dst; \
    }

#define LOCAL_MOD(type, op) \
    case type: { \
        LOC_RAW_DIV(op) \
        this->TryCast(dst, x86::Gp::rdx); \
        return dst; \
    }

#define GLOBAL_DIV_RAW(op) \
    this->TryCast(x86::Gp::rax, dst); \
    this->writer->xor_(x86::Gp::rdx, x86::Gp::rdx); \
    if (freg) { \
        this->writer->op(*freg); \
    } else { \
        this->writer->mov(x86::Gp::r15, this->writer->ptr( \
            binop->right()->as<IRGlobRef>()->getName(), \
            0, \
            binop->right()->as<IRGlobRef>()->getType()->moveSize() \
        )); \
        this->writer->op(x86::Gp::r15); \
    } \

#define GLOBAL_DIV(type, op) \
    case type: { \
        GLOBAL_DIV_RAW(op) \
        this->TryCast(dst, x86::Gp::rax); \
        return dst; \
    }

#define GLOBAL_MOD(type, op) \
    case type: { \
        GLOBAL_DIV_RAW(op) \
        this->TryCast(dst, x86::Gp::rdx); \
        return dst; \
    }

#define REGS_RAW_DIV(op) \
    this->TryCast(x86::Gp::rax, dst); \
    this->writer->xor_(x86::Gp::rdx, x86::Gp::rdx); \
    this->TryCast(x86::Gp::rdx, tmp); \
    this->writer->op(tmp); \

#define REGS_DIV() \
    if (dst.signed_value) { \
        REGS_RAW_DIV(idiv) \
    } else { \
        REGS_RAW_DIV(div) \
    } \
    this->TryCast(dst, x86::Gp::rax); \

#define REGS_MOD() \
    if (dst.signed_value) { \
        REGS_RAW_DIV(idiv) \
    } else { \
        REGS_RAW_DIV(div) \
    } \
    this->TryCast(dst, x86::Gp::rdx); \

Reg WindEmitter::EmitBinOp(IRBinOp *binop, Reg dst, bool isJmp) {
    uint8_t tmp_size = dst.size;
    if (binop->operation() == IRBinOp::Operation::L_ASSIGN
        || binop->operation() == IRBinOp::Operation::L_PLUS_ASSIGN
        || binop->operation() == IRBinOp::Operation::L_MINUS_ASSIGN) {
        tmp_size = binop->left()->as<IRLocalRef>()->datatype()->moveSize();
    } else if (binop->operation() == IRBinOp::Operation::G_ASSIGN
        || binop->operation() == IRBinOp::Operation::G_PLUS_ASSIGN
        || binop->operation() == IRBinOp::Operation::G_MINUS_ASSIGN) {
        tmp_size = binop->left()->as<IRGlobRef>()->getType()->moveSize();
    }
    else {
        dst = this->EmitExpr((IRNode*)binop->left(), dst);
        this->regalloc.SetDirty(dst);
    }
    

    switch (binop->right()->type()) {
        case IRNode::NodeType::LITERAL: {
            switch (binop->operation()) {
                LITERAL_OP(IRBinOp::ADD, add)
                LITERAL_OP(IRBinOp::SUB, sub)
                LITERAL_OP(IRBinOp::AND, and_)
                LIT_CMP_OP(IRBinOp::EQ, sete)
                GEN_ASSIGN_LIT(IRBinOp::L_ASSIGN, L_ASSIGN_OP)
                GEN_ASSIGN_LIT(IRBinOp::G_ASSIGN, G_ASSIGN_OP)
                GEN_ASSIGN_LIT(IRBinOp::L_PLUS_ASSIGN, L_PLUS_ASSIGN_OP)
                GEN_ASSIGN_LIT(IRBinOp::G_PLUS_ASSIGN, G_PLUS_ASSIGN_OP)
                GEN_ASSIGN_LIT(IRBinOp::L_MINUS_ASSIGN, L_MINUS_ASSIGN_OP)
                GEN_ASSIGN_LIT(IRBinOp::G_MINUS_ASSIGN, G_MINUS_ASSIGN_OP)
                default: {}
            }
            if (dst.signed_value) {
                switch (binop->operation()) {
                    LITERAL_OP(IRBinOp::SHL, sal)
                    LITERAL_OP(IRBinOp::SHR, sar)
                    LITERAL_OP(IRBinOp::MUL, imul)
                    LIT_DIV(IRBinOp::DIV, idiv)
                    LIT_MOD(IRBinOp::MOD, idiv)
                    LIT_CMP_OP(IRBinOp::LESS, setl)
                    LIT_CMP_OP(IRBinOp::GREATER, setg)
                    LIT_CMP_OP(IRBinOp::LESSEQ, setle)
                }
            } else {
                switch (binop->operation()) {
                    LITERAL_OP(IRBinOp::MUL, mul)
                    LITERAL_OP(IRBinOp::SHL, shl)
                    LITERAL_OP(IRBinOp::SHR, shr)
                    LIT_DIV(IRBinOp::DIV, div)
                    LIT_MOD(IRBinOp::MOD, div)
                    LIT_CMP_OP(IRBinOp::LESS, setb)
                    LIT_CMP_OP(IRBinOp::GREATER, seta)
                    LIT_CMP_OP(IRBinOp::LESSEQ, setbe)
                }
            }
            break;
        }
        case IRNode::NodeType::LOCAL_REF: {
            Reg *freg = this->regalloc.FindLocalVar(binop->right()->as<IRLocalRef>()->offset(), binop->right()->as<IRLocalRef>()->datatype()->moveSize());
            dst = this->CastReg(dst, binop->right()->as<IRLocalRef>()->datatype()->moveSize());
            switch (binop->operation()) {
                OPT_LOCAL_OP(IRBinOp::ADD, add)
                OPT_LOCAL_OP(IRBinOp::SUB, sub)
                OPT_LOCAL_OP(IRBinOp::AND, and_)
                LOC_CMP_OP(IRBinOp::EQ, sete)
                default: {}
            }
            if (dst.signed_value) {
                switch (binop->operation()) {
                    OPT_LOCAL_OP(IRBinOp::SHL, sal)
                    OPT_LOCAL_OP(IRBinOp::SHR, sar)
                    OPT_LOCAL_OP(IRBinOp::MUL, imul)
                    LOCAL_DIV(IRBinOp::DIV, idiv)
                    LOCAL_MOD(IRBinOp::MOD, idiv)
                    LOC_CMP_OP(IRBinOp::LESS, setl)
                    LOC_CMP_OP(IRBinOp::GREATER, setg)
                    LOC_CMP_OP(IRBinOp::LESSEQ, setle)
                }
            } else {
                switch (binop->operation()) {
                    OPT_LOCAL_OP(IRBinOp::SHL, shl)
                    OPT_LOCAL_OP(IRBinOp::SHR, shr)
                    OPT_LOCAL_OP(IRBinOp::MUL, mul)
                    LOCAL_DIV(IRBinOp::DIV, div)
                    LOCAL_MOD(IRBinOp::MOD, div)
                    LOC_CMP_OP(IRBinOp::LESS, setb)
                    LOC_CMP_OP(IRBinOp::GREATER, seta)
                    LOC_CMP_OP(IRBinOp::LESSEQ, setbe)
                }
            }
            break;
        }
        case IRNode::NodeType::GLOBAL_REF: {
            Reg *freg = this->regalloc.FindLabel(binop->right()->as<IRGlobRef>()->getName(), binop->right()->as<IRGlobRef>()->getType()->moveSize());
            dst = this->CastReg(dst, binop->right()->as<IRGlobRef>()->getType()->moveSize());
            switch (binop->operation()) {
                GLOBAL_OP(IRBinOp::ADD, add)
                GLOBAL_OP(IRBinOp::SUB, sub)
                GLOBAL_OP(IRBinOp::AND, and_)
                GLB_CMP_OP(IRBinOp::EQ, sete)
                default: {}
            }
            if (dst.signed_value) {
                switch (binop->operation()) {
                    GLOBAL_OP(IRBinOp::SHL, sal)
                    GLOBAL_OP(IRBinOp::SHR, sar)
                    GLOBAL_OP(IRBinOp::MUL, imul)
                    GLOBAL_DIV(IRBinOp::DIV, idiv)
                    GLOBAL_MOD(IRBinOp::MOD, idiv)
                    GLB_CMP_OP(IRBinOp::LESS, setl)
                    GLB_CMP_OP(IRBinOp::GREATER, setg)
                    GLB_CMP_OP(IRBinOp::LESSEQ, setle)
                }
            } else {
                switch (binop->operation()) {
                    GLOBAL_OP(IRBinOp::SHL, shl)
                    GLOBAL_OP(IRBinOp::SHR, shr)
                    GLOBAL_OP(IRBinOp::MUL, mul)
                    GLOBAL_DIV(IRBinOp::DIV, div)
                    GLOBAL_MOD(IRBinOp::MOD, div)
                    GLB_CMP_OP(IRBinOp::LESS, setb)
                    GLB_CMP_OP(IRBinOp::GREATER, seta)
                    GLB_CMP_OP(IRBinOp::LESSEQ, setbe)
                }
            }
            break;
        }
        default: {}
    }

    Reg tmp = this->regalloc.Allocate(tmp_size, false);
    tmp = this->EmitExpr((IRNode*)binop->right(), tmp);
    this->regalloc.SetDirty(tmp);

    switch (binop->operation()) {
        case IRBinOp::ADD:
            this->writer->add(dst, tmp);
            break;
        case IRBinOp::SUB:
            this->writer->sub(dst, tmp);
            break;
        case IRBinOp::SHL:
            if (dst.signed_value) { this->writer->sal(dst, tmp); }
            else { this->writer->shl(dst, tmp); }
            break;
        case IRBinOp::SHR:
            if (dst.signed_value) { this->writer->sar(dst, tmp); }
            else { this->writer->shr(dst, tmp); }
            break;
        case IRBinOp::MUL:
            if (dst.signed_value) { this->writer->imul(dst, tmp); }
            else { this->writer->mul(dst, tmp); }
            break;
        case IRBinOp::DIV:
            REGS_DIV()
            break;
        case IRBinOp::MOD:
            REGS_MOD()
            break;
        case IRBinOp::AND:
            this->writer->and_(dst, tmp);
            break;
        case IRBinOp::L_ASSIGN:
            L_ASSIGN_OP(tmp)
            break;
        case IRBinOp::G_ASSIGN:
            G_ASSIGN_OP(tmp)
            break;
        case IRBinOp::L_PLUS_ASSIGN:
            L_PLUS_ASSIGN_OP(tmp)
            break;
        case IRBinOp::G_PLUS_ASSIGN:
            G_PLUS_ASSIGN_OP(tmp)
            break;
        case IRBinOp::L_MINUS_ASSIGN:
            L_MINUS_ASSIGN_OP(tmp)
            break;
        case IRBinOp::G_MINUS_ASSIGN:
            G_MINUS_ASSIGN_OP(tmp)
            break;
        case IRBinOp::EQ:
            EL_CMP_OP(sete)
            break;
        case IRBinOp::LESS:
            if (dst.signed_value) {
                EL_CMP_OP(setl)
            } else {
                EL_CMP_OP(setb)
            }
            break;
        case IRBinOp::GREATER:
            if (dst.signed_value) {
                EL_CMP_OP(setg)
            } else {
                EL_CMP_OP(seta)
            }
            break;
        case IRBinOp::LESSEQ:
            if (dst.signed_value) {
                EL_CMP_OP(setle)
            } else {
                EL_CMP_OP(setbe)
            }
            break;
        default:
            throw std::runtime_error("Unsupported binop (" + std::to_string((uint8_t)binop->operation()) + "): Report to maintainer!");
    }

    dst.signed_value = tmp.signed_value ? tmp.signed_value : dst.signed_value;
    this->regalloc.Free(tmp);
    if (binop->operation() == IRBinOp::Operation::L_ASSIGN) {
        this->regalloc.SetVar(dst, binop->left()->as<IRLocalRef>()->offset(), RegisterAllocator::RegValue::Lifetime::UNTIL_ALLOC);
    } else if (binop->operation() == IRBinOp::Operation::G_ASSIGN) {
        this->regalloc.SetLabel(dst, binop->left()->as<IRGlobRef>()->getName(), RegisterAllocator::RegValue::Lifetime::UNTIL_ALLOC);
    }
    return dst;
}

Reg WindEmitter::EmitExpr(IRNode *value, Reg dst, bool isJmp) {
    if (!this->regalloc.Request(dst)) {
        std::cerr << this->GetAsm() << std::endl;
        this->regalloc.AllocRepr();
        throw std::runtime_error("Register allocation failed");
    }

    bool jmpProcessed=false;
    Reg res_reg;
    switch (value->type()) {
        case IRNode::NodeType::BIN_OP: {
            res_reg = this->EmitBinOp(value->as<IRBinOp>(), dst, isJmp);
            jmpProcessed=true;
            break;
        }
        case IRNode::NodeType::FUNCTION_CALL: {
            res_reg = this->EmitFnCall(value->as<IRFnCall>(), dst);
            break;
        }
        default: {
            res_reg = this->EmitValue(value, dst);
        }
    }

    this->regalloc.PostExpression();
    return res_reg;
}