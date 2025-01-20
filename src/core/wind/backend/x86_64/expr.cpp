#include <wind/generation/IR.h>
#include <wind/backend/writer/writer.h>
#include <wind/backend/x86_64/backend.h>
#include <wind/backend/x86_64/expr_macros.h>
#include <wind/bridge/flags.h>
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
        case IRNode::NodeType::GENERIC_INDEXING: {
            return this->EmitGenAddrRef(value->as<IRGenericIndexing>(), dst);
        }
        case IRNode::NodeType::FN_REF: {
            return this->EmitFnRef(value->as<IRFnRef>(), dst);
        }
        case IRNode::NodeType::PTR_GUARD: {
            return this->EmitPtrGuard(value->as<IRPtrGuard>(), dst);
        }
        case IRNode::NodeType::TYPE_CAST: {
            return this->EmitTypeCast(value->as<IRTypeCast>(), dst);
        }
        default: {
            throw std::runtime_error("Unknown value type(" + std::to_string((uint8_t)value->type()) + "): Report to maintainer!");
        }
    }
    return dst;
}

void WindEmitter::TryCast(Reg dst, Reg proc) {
    if (dst.size > proc.size && proc.size < 4) {
        if (!dst.signed_value) {
            this->writer->movzx(dst, proc);
        } else {
            this->writer->movsx(dst, proc);
        }
    } else if (dst.id != proc.id) {
        this->writer->mov(dst, proc);
    }
}

Reg WindEmitter::EmitBinOp(IRBinOp *binop, Reg dst, bool isJmp) {
    /*
    TODO:
    */
    uint8_t tmp_size = dst.size;
    if (binop->operation() == IRBinOp::Operation::L_ASSIGN
        || binop->operation() == IRBinOp::Operation::L_PLUS_ASSIGN
        || binop->operation() == IRBinOp::Operation::L_MINUS_ASSIGN) {
        tmp_size = binop->left()->as<IRLocalRef>()->datatype()->moveSize();
    } else if (binop->operation() == IRBinOp::Operation::G_ASSIGN
        || binop->operation() == IRBinOp::Operation::G_PLUS_ASSIGN
        || binop->operation() == IRBinOp::Operation::G_MINUS_ASSIGN) {
        tmp_size = binop->left()->as<IRGlobRef>()->getType()->moveSize();
    } else if (binop->operation() == IRBinOp::Operation::VA_ASSIGN) {
        tmp_size = binop->left()->as<IRLocalAddrRef>()->datatype()->moveSize();
    }
    else {
        dst = this->EmitExpr((IRNode*)binop->left(), dst);
        this->regalloc.SetDirty(dst);
    }
    

    switch (binop->right()->type()) {
        case IRNode::NodeType::LITERAL: {
            switch (binop->operation()) {
                LITERAL_OP(IRBinOp::ADD, WRITER_ADD)
                LITERAL_OP(IRBinOp::SUB, WRITER_SUB)
                LITERAL_OP(IRBinOp::AND, WRITER_AND)
                LITERAL_OP(IRBinOp::XOR, WRITER_XOR)
                LITERAL_OP(IRBinOp::OR, WRITER_OR)
                LIT_CMP_OP(IRBinOp::EQ, sete)
                LIT_CMP_OP(IRBinOp::NOTEQ, setne)
                GEN_ASSIGN_LIT(IRBinOp::L_ASSIGN, L_ASSIGN_OP)
                GEN_ASSIGN_LIT(IRBinOp::G_ASSIGN, G_ASSIGN_OP)
                GEN_ASSIGN_LIT(IRBinOp::L_PLUS_ASSIGN, L_PLUS_ASSIGN_OP)
                GEN_ASSIGN_LIT(IRBinOp::G_PLUS_ASSIGN, G_PLUS_ASSIGN_OP)
                GEN_ASSIGN_LIT(IRBinOp::L_MINUS_ASSIGN, L_MINUS_ASSIGN_OP)
                GEN_ASSIGN_LIT(IRBinOp::G_MINUS_ASSIGN, G_MINUS_ASSIGN_OP)
                //LIT_VA_ASSIGN(IRBinOp::VA_ASSIGN) // TODO
                default: {}
            }
            if (dst.signed_value) {
                switch (binop->operation()) {
                    LITERAL_OP(IRBinOp::SHL, WRITER_SAL)
                    LITERAL_OP(IRBinOp::SHR, WRITER_SAR)
                    LITERAL_OP(IRBinOp::MUL, WRITER_IMUL)
                    LIT_DIV(IRBinOp::DIV, WRITER_DIV)
                    LIT_MOD(IRBinOp::MOD, WRITER_IDIV)
                    LIT_CMP_OP(IRBinOp::LESS, setl)
                    LIT_CMP_OP(IRBinOp::GREATER, setg)
                    LIT_CMP_OP(IRBinOp::LESSEQ, setle)
                    LIT_CMP_OP(IRBinOp::GREATEREQ, setge)
                }
            } else {
                switch (binop->operation()) {
                    LITERAL_OP(IRBinOp::MUL, WRITER_MUL)
                    LITERAL_OP(IRBinOp::SHL, WRITER_SHL)
                    LITERAL_OP(IRBinOp::SHR, WRITER_SHR)
                    LIT_DIV(IRBinOp::DIV, WRITER_DIV)
                    LIT_MOD(IRBinOp::MOD, WRITER_DIV)
                    LIT_CMP_OP(IRBinOp::LESS, setb)
                    LIT_CMP_OP(IRBinOp::GREATER, seta)
                    LIT_CMP_OP(IRBinOp::LESSEQ, setbe)
                    LIT_CMP_OP(IRBinOp::GREATEREQ, setae)
                }
            }
            break;
        }
        case IRNode::NodeType::LOCAL_REF: {
            Reg *freg = this->regalloc.FindLocalVar(binop->right()->as<IRLocalRef>()->offset(), binop->right()->as<IRLocalRef>()->datatype()->moveSize());
            //dst = this->CastReg(dst, binop->right()->as<IRLocalRef>()->datatype()->moveSize());
            switch (binop->operation()) {
                OPT_LOCAL_OP(IRBinOp::ADD, WRITER_ADD)
                OPT_LOCAL_OP(IRBinOp::SUB, WRITER_SUB)
                OPT_LOCAL_OP(IRBinOp::AND, WRITER_AND)
                OPT_LOCAL_OP(IRBinOp::XOR, WRITER_XOR)
                OPT_LOCAL_OP(IRBinOp::OR, WRITER_OR)
                LOC_CMP_OP(IRBinOp::EQ, sete)
                LOC_CMP_OP(IRBinOp::NOTEQ, setne)
                default: {}
            }
            if (dst.signed_value) {
                switch (binop->operation()) {
                    OPT_LOCAL_OP(IRBinOp::SHL, WRITER_SAL)
                    OPT_LOCAL_OP(IRBinOp::SHR, WRITER_SAR)
                    OPT_LOCAL_OP(IRBinOp::MUL, WRITER_IMUL)
                    LOCAL_DIV(IRBinOp::DIV, WRITER_IDIV)
                    LOCAL_MOD(IRBinOp::MOD, WRITER_IDIV)
                    LOC_CMP_OP(IRBinOp::LESS, setl)
                    LOC_CMP_OP(IRBinOp::GREATER, setg)
                    LOC_CMP_OP(IRBinOp::LESSEQ, setle)
                    LOC_CMP_OP(IRBinOp::GREATEREQ, setge)
                }
            } else {
                switch (binop->operation()) {
                    OPT_LOCAL_OP(IRBinOp::SHL, WRITER_SHL)
                    OPT_LOCAL_OP(IRBinOp::SHR, WRITER_SHR)
                    OPT_LOCAL_OP(IRBinOp::MUL, WRITER_MUL)
                    LOCAL_DIV(IRBinOp::DIV, WRITER_DIV)
                    LOCAL_MOD(IRBinOp::MOD, WRITER_DIV)
                    LOC_CMP_OP(IRBinOp::LESS, setb)
                    LOC_CMP_OP(IRBinOp::GREATER, seta)
                    LOC_CMP_OP(IRBinOp::LESSEQ, setbe)
                    LOC_CMP_OP(IRBinOp::GREATEREQ, setae)
                }
            }
            break;
        }
        case IRNode::NodeType::GLOBAL_REF: {
            Reg *freg = this->regalloc.FindLabel(binop->right()->as<IRGlobRef>()->getName(), binop->right()->as<IRGlobRef>()->getType()->moveSize());
            //dst = this->CastReg(dst, binop->right()->as<IRGlobRef>()->getType()->moveSize());
            switch (binop->operation()) {
                GLOBAL_OP(IRBinOp::ADD, WRITER_ADD)
                GLOBAL_OP(IRBinOp::SUB, WRITER_SUB)
                GLOBAL_OP(IRBinOp::AND, WRITER_AND)
                GLOBAL_OP(IRBinOp::XOR, WRITER_XOR)
                GLOBAL_OP(IRBinOp::OR, WRITER_OR)
                GLB_CMP_OP(IRBinOp::EQ, sete)
                GLB_CMP_OP(IRBinOp::NOTEQ, setne)
                default: {}
            }
            if (dst.signed_value) {
                switch (binop->operation()) {
                    GLOBAL_OP(IRBinOp::SHL, WRITER_SAL)
                    GLOBAL_OP(IRBinOp::SHR, WRITER_SAR)
                    GLOBAL_OP(IRBinOp::MUL, WRITER_IMUL)
                    GLOBAL_DIV(IRBinOp::DIV, WRITER_IDIV)
                    GLOBAL_MOD(IRBinOp::MOD, WRITER_IDIV)
                    GLB_CMP_OP(IRBinOp::LESS, setl)
                    GLB_CMP_OP(IRBinOp::GREATER, setg)
                    GLB_CMP_OP(IRBinOp::LESSEQ, setle)
                    GLB_CMP_OP(IRBinOp::GREATEREQ, setge)
                }
            } else {
                switch (binop->operation()) {
                    GLOBAL_OP(IRBinOp::SHL, WRITER_SHL)
                    GLOBAL_OP(IRBinOp::SHR, WRITER_SHR)
                    GLOBAL_OP(IRBinOp::MUL, WRITER_MUL)
                    GLOBAL_DIV(IRBinOp::DIV, WRITER_DIV)
                    GLOBAL_MOD(IRBinOp::MOD, WRITER_DIV)
                    GLB_CMP_OP(IRBinOp::LESS, setb)
                    GLB_CMP_OP(IRBinOp::GREATER, seta)
                    GLB_CMP_OP(IRBinOp::LESSEQ, setbe)
                    GLB_CMP_OP(IRBinOp::GREATEREQ, setae)
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
        case IRBinOp::ADD: {
            WRITER_ADD(dst, tmp);
            break;
        }
        case IRBinOp::SUB: {
            WRITER_SUB(dst, tmp);
            break;
        }
        case IRBinOp::SHL: {
            if (dst.signed_value) { this->writer->sal(dst, tmp); }
            else { this->writer->shl(dst, tmp); }
            break;
        }
        case IRBinOp::SHR: {
            if (dst.signed_value) { this->writer->sar(dst, tmp); }
            else { this->writer->shr(dst, tmp); }
            break;
        }
        case IRBinOp::MUL: {
            if (dst.signed_value) { WRITER_IMUL(dst, tmp); }
            else { WRITER_MUL(dst, tmp); }
            break;
        }
        case IRBinOp::DIV: {
            REGS_DIV()
            break;
        }
        case IRBinOp::MOD: {
            REGS_MOD()
            break;
        }
        case IRBinOp::AND: {
            this->writer->and_(dst, tmp);
            break;
        }
        case IRBinOp::XOR: {
            this->writer->xor_(dst, tmp);
            break;
        }
        case IRBinOp::OR: {
            this->writer->or_(dst, tmp);
            break;
        }
        case IRBinOp::L_ASSIGN: {
            L_ASSIGN_OP(
                CastReg(tmp, binop->left()->as<IRLocalRef>()->datatype()->moveSize())
            )
            break;
        }
        case IRBinOp::G_ASSIGN: {
            G_ASSIGN_OP(
                CastReg(tmp, binop->left()->as<IRGlobRef>()->getType()->moveSize())
            )
            break;
        }
        case IRBinOp::VA_ASSIGN: {
            this->EmitIntoLocAddrRef((IRLocalAddrRef*)binop->left()->as<IRLocalAddrRef>(), tmp);
            break;
        }
        case IRBinOp::GEN_INDEX_ASSIGN: {
            this->EmitIntoGenAddrRef((IRGenericIndexing*)binop->left()->as<IRGenericIndexing>(), tmp);
            break;
        }
        case IRBinOp::EQ: {
            EL_CMP_OP(sete)
            break;
        }
        case IRBinOp::NOTEQ: {
            EL_CMP_OP(setne)
            break;
        }
        case IRBinOp::LESS: {
            if (dst.signed_value) {
                EL_CMP_OP(setl)
            } else {
                EL_CMP_OP(setb)
            }
            break;
        }
        case IRBinOp::GREATER: {
            if (dst.signed_value) {
                EL_CMP_OP(setg)
            } else {
                EL_CMP_OP(seta)
            }
            break;
        }
        case IRBinOp::LESSEQ: {
            if (dst.signed_value) {
                EL_CMP_OP(setle)
            } else {
                EL_CMP_OP(setbe)
            }
            break;
        }
        case IRBinOp::GREATEREQ: {
            if (dst.signed_value) {
                EL_CMP_OP(setge)
            } else {
                EL_CMP_OP(setae)
            }
            break;
        }
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