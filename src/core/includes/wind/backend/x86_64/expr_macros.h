#ifndef BACKEND_EXPR_MACROS
#define BACKEND_EXPR_MACROS

#define CASTED_MOV(dst, src) \
    if (dst.size > src.size) { \
        if (!dst.signed_value) { \
            this->writer->movzx(dst, src); \
        } else { \
            this->writer->movsx(dst, src); \
        } \
    } else if (dst.id != src.id) { \
        this->writer->mov(dst, src); \
    }

#define LITERAL_OP(type, op) \
    case type: { \
        op(dst, binop->right()->as<IRLiteral>()->get()); \
        return dst; \
    }


#define LOCAL_OP_RAW(op) \
    op(dst, this->writer->ptr( \
        x86::Gp::rbp, \
        binop->right()->as<IRLocalRef>()->offset(), \
        dst.size \
    )); \
    return dst;

#define OPT_LOCAL_OP(type, op) \
    case type: { \
        if (freg) { \
            op(dst, *freg); \
            dst.signed_value = binop->right()->as<IRLocalRef>()->datatype()->isSigned(); \
            return dst; \
        } else { \
            LOCAL_OP_RAW(op) \
        } \
    }

#define GLOB_OP_RAW(op) \
    op(dst, this->writer->ptr( \
        binop->right()->as<IRGlobRef>()->getName(), \
        0, \
        dst.size \
    )); \
    return dst;

#define GLOBAL_OP(type, op) \
    case type: { \
        if (freg) { \
            op(dst, *freg); \
            return dst; \
        } else { \
            GLOB_OP_RAW(op) \
        } \
    }

#define L_ASSIGN_OP(src) \
    this->writer->mov( \
        this->writer->ptr( \
            x86::Gp::rbp, \
            binop->left()->as<IRLocalRef>()->offset(), \
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
    WRITER_ADD( \
        this->writer->ptr( \
            x86::Gp::rbp, \
            binop->left()->as<IRLocalRef>()->offset(), \
            binop->left()->as<IRLocalRef>()->datatype()->moveSize() \
        ), \
        src \
    ); \

#define G_PLUS_ASSIGN_OP(src) \
    WRITER_ADD( \
        this->writer->ptr( \
            binop->left()->as<IRGlobRef>()->getName(), \
            0, \
            binop->left()->as<IRGlobRef>()->getType()->moveSize() \
        ), \
        src \
    ); \

#define L_MINUS_ASSIGN_OP(src) \
    WRITER_SUB( \
        this->writer->ptr( \
            x86::Gp::rbp, \
            binop->left()->as<IRLocalRef>()->offset(), \
            binop->left()->as<IRLocalRef>()->datatype()->moveSize() \
        ), \
        src \
    ); \

#define G_MINUS_ASSIGN_OP(src) \
    WRITER_SUB( \
        this->writer->ptr( \
            binop->left()->as<IRGlobRef>()->getName(), \
            0, \
            binop->left()->as<IRGlobRef>()->getType()->moveSize() \
        ), \
        src \
    ); \


#define VA_ASSIGN_OP(src) \
    const IRLocalAddrRef *ref = binop->left()->as<IRLocalAddrRef>(); \
    if (ref->isIndexed()) { \
        if (!ref->datatype()->isArray()) { \
            throw std::runtime_error("Cannot index non-array"); \
        } \
        if (ref->getIndex()->type() == IRNode::NodeType::LITERAL) { \
            int16_t offset = ref->offset() - ref->datatype()->index2offset(ref->getIndex()->as<IRLiteral>()->get()); \
            if (offset < 0) { \
                throw std::runtime_error("Invalid offset"); \
            } \
            src = this->CastReg(src, ref->datatype()->rawSize()); \
            this->writer->mov( \
                this->writer->ptr( \
                    x86::Gp::rbp, \
                    offset, \
                    ref->datatype()->rawSize() \
                ), \
                src, \
                ref->datatype()->rawSize() \
            ); \
        } \
        else { \
            Reg r_index = this->EmitValue(ref->getIndex(), this->regalloc.Allocate(8, false)); \
            Reg index = {r_index.id, 8, Reg::GPR, false}; \
            this->TryCast(index, r_index); \
            src = this->CastReg(src, ref->datatype()->rawSize()); \
            this->writer->mov( \
                this->writer->ptr( \
                    x86::Gp::rbp, \
                    index, \
                    ref->offset(), \
                    ref->datatype()->rawSize() \
                ), \
                src, \
                ref->datatype()->rawSize() \
            ); \
        } \
    }

#define VA_OPASSIGN_OP(src, op) \
    const IRLocalAddrRef *ref = binop->left()->as<IRLocalAddrRef>(); \
    if (ref->isIndexed()) { \
        if (!ref->datatype()->isArray()) { \
            throw std::runtime_error("Cannot index non-array"); \
        } \
        if (ref->getIndex()->type() == IRNode::NodeType::LITERAL) { \
            int16_t offset = ref->offset() - ref->datatype()->index2offset(ref->getIndex()->as<IRLiteral>()->get()); \
            if (offset < 0) { \
                throw std::runtime_error("Invalid offset"); \
            } \
            src = this->CastReg(src, ref->datatype()->rawSize()); \
            this->writer->op( \
                this->writer->ptr( \
                    x86::Gp::rbp, \
                    offset, \
                    ref->datatype()->rawSize() \
                ), \
                src \
            ); \
            this->writer->mov( \
                this->writer->ptr( \
                    x86::Gp::rbp, \
                    offset, \
                    ref->datatype()->rawSize() \
                ), \
                src, \
                ref->datatype()->rawSize() \
            ); \
        } \
        else { \
            Reg r_index = this->EmitValue(ref->getIndex(), dst); \
            Reg index = {r_index.id, 8, Reg::GPR, false}; \
            this->TryCast(index, r_index); \
            src = this->CastReg(src, ref->datatype()->rawSize()); \
            this->writer->op( \
                this->writer->ptr( \
                    x86::Gp::rbp, \
                    index, \
                    ref->offset(), \
                    ref->datatype()->rawSize() \
                ), \
                src \
            ); \
            this->writer->mov( \
                this->writer->ptr( \
                    x86::Gp::rbp, \
                    index, \
                    ref->offset(), \
                    ref->datatype()->rawSize() \
                ), \
                src, \
                ref->datatype()->rawSize() \
            ); \
        } \
    } \

#define RAW_VALIT_ASSIGN(src) \
    const IRLocalAddrRef *ref = binop->left()->as<IRLocalAddrRef>(); \
    if (ref->isIndexed()) { \
        if (!ref->datatype()->isArray()) { \
            throw std::runtime_error("Cannot index non-array"); \
        } \
        if (ref->getIndex()->type() == IRNode::NodeType::LITERAL) { \
            int16_t offset = ref->offset() - ref->datatype()->index2offset(ref->getIndex()->as<IRLiteral>()->get()); \
            if (offset < 0) { \
                throw std::runtime_error("Invalid offset"); \
            } \
            this->writer->mov( \
                this->writer->ptr( \
                    x86::Gp::rbp, \
                    offset, \
                    ref->datatype()->rawSize() \
                ), \
                src \
            ); \
        } \
        else { \
            Reg r_index = this->EmitValue(ref->getIndex(), dst); \
            Reg index = {r_index.id, 8, Reg::GPR, false}; \
            this->TryCast(index, r_index); \
            this->writer->mov( \
                this->writer->ptr( \
                    x86::Gp::rbp, \
                    index, \
                    ref->offset(), \
                    ref->datatype()->rawSize() \
                ), \
                src \
            ); \
        } \
    }

#define LIT_VA_ASSIGN(type) \
    case type: { \
        RAW_VALIT_ASSIGN(binop->right()->as<IRLiteral>()->get()); \
        return dst; \
    }


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
            LOCAL_OP_RAW(WRITER_CMP) \
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
    CASTED_MOV(dst, this->writer->ptr( \
        x86::Gp::rbp, \
        binop->left()->as<IRLocalRef>()->offset(), \
        binop->left()->as<IRLocalRef>()->datatype()->moveSize() \
    ));

#define AFTER_GLB_ASSIGN() \
    dst.size = binop->left()->as<IRGlobRef>()->getType()->moveSize(); \
    dst.signed_value = binop->left()->as<IRGlobRef>()->getType()->isSigned(); \
    CASTED_MOV(dst, this->writer->ptr( \
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
        else if (binop->left()->is<IRLocalAddrRef>()) {} \
        else {\
            AFTER_GLB_ASSIGN() \
            this->regalloc.SetLabel(dst, binop->left()->as<IRGlobRef>()->getName(), RegisterAllocator::RegValue::Lifetime::UNTIL_ALLOC); \
        } \
        return dst; \
    }

#define CQ_GEN(reg) \
    if (!reg.signed_value)  { \
        this->writer->xor_(x86::Gp::rdx, x86::Gp::rdx); \
    } \
    if (reg.size == 8) { \
        this->writer->cqo(); \
    } else if (reg.size == 4) { \
        this->writer->cdq(); \
    } else { \
        this->writer->xor_(x86::Gp::rdx, x86::Gp::rdx); \
    }

#define LIT_RAW_DIV(op) \
    this->TryCast(x86::Gp::rax, dst); \
    Reg saved_rdx; bool d_rdx = this->regalloc.isDirty(x86::Gp::rdx); \
    if (d_rdx) { \
        saved_rdx = this->regalloc.Allocate(8, true); \
        this->writer->mov(saved_rdx, x86::Gp::rdx); \
    } \
    this->regalloc.SetDirty(x86::Gp::rdx); \
    Reg tmp = this->EmitExpr((IRNode*)binop->right(), this->regalloc.Allocate(8, false)); \
    CQ_GEN(dst) \
    op(tmp); \
    this->regalloc.Free(tmp); \
    if (d_rdx) { \
        this->writer->mov(x86::Gp::rdx, saved_rdx); \
        this->regalloc.Free(saved_rdx); \
    } \

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
    Reg saved_rdx; bool d_rdx = this->regalloc.isDirty(x86::Gp::rdx); \
    if (d_rdx) { \
        saved_rdx = this->regalloc.Allocate(8, true); \
        this->writer->mov(saved_rdx, x86::Gp::rdx); \
    } \
    CQ_GEN(dst) \
    if (freg) { \
        op(*freg); \
    } else { \
        op( \
            this->writer->ptr( \
                x86::Gp::rbp, \
                binop->right()->as<IRLocalRef>()->offset(), \
                binop->right()->as<IRLocalRef>()->datatype()->moveSize() \
            ) \
        ) \
    } \
    if (d_rdx) { \
        this->writer->mov(x86::Gp::rdx, saved_rdx); \
        this->regalloc.Free(saved_rdx); \
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
    Reg saved_rdx; bool d_rdx = this->regalloc.isDirty(x86::Gp::rdx); \
    if (d_rdx) { \
        saved_rdx = this->regalloc.Allocate(8, true); \
        this->writer->mov(saved_rdx, x86::Gp::rdx); \
    } \
    CQ_GEN(dst) \
    if (freg) { \
        op(*freg); \
    } else { \
        op( \
            this->writer->ptr( \
                binop->right()->as<IRGlobRef>()->getName(), \
                0, \
                binop->right()->as<IRGlobRef>()->getType()->moveSize() \
            ) \
        ) \
    } \
    if (d_rdx) { \
        this->writer->mov(x86::Gp::rdx, saved_rdx); \
        this->regalloc.Free(saved_rdx); \
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
    Reg saved_rdx; bool d_rdx = this->regalloc.isDirty(x86::Gp::rdx); \
    if (d_rdx) { \
        saved_rdx = this->regalloc.Allocate(8, true); \
        this->writer->mov(saved_rdx, x86::Gp::rdx); \
    } \
    CQ_GEN(dst) \
    this->TryCast(x86::Gp::rdx, tmp); \
    op(tmp); \
    if (d_rdx) { \
        this->writer->mov(x86::Gp::rdx, saved_rdx); \
        this->regalloc.Free(saved_rdx); \
    } \

#define REGS_DIV() \
    if (dst.signed_value) { \
        REGS_RAW_DIV(WRITER_IDIV) \
    } else { \
        REGS_RAW_DIV(WRITER_DIV) \
    } \
    this->TryCast(dst, x86::Gp::rax); \

#define REGS_MOD() \
    if (dst.signed_value) { \
        REGS_RAW_DIV(WRITER_IDIV) \
    } else { \
        REGS_RAW_DIV(WRITER_DIV) \
    } \
    this->TryCast(dst, x86::Gp::rdx); \

#define HANDLED_2OP(op, id, rdst, rsrc) \
    this->writer->op(rdst, rsrc, GetHandlerLabel(#op));

#define HANDLED_1OP(op, id, rsrc) \
    this->writer->op(rsrc, GetHandlerLabel(#op));

#define WRITER_ADD(rdst, rsrc) HANDLED_2OP(add, add, rdst, rsrc)
#define WRITER_SUB(rdst, rsrc) HANDLED_2OP(sub, sub, rdst, rsrc)
#define WRITER_MUL(rdst, rsrc) HANDLED_2OP(imul, mul, rdst, rsrc)
#define WRITER_IMUL(rdst, rsrc) HANDLED_2OP(imul, imul, rdst, rsrc)
#define WRITER_DIV(rsrc) HANDLED_1OP(div, div, rsrc)
#define WRITER_IDIV(rsrc) HANDLED_1OP(idiv, idiv, rsrc)
#define WRITER_MOD(rdst, rsrc) HANDLED_2OP(idiv, div, rdst, rsrc)


#define WRITER_AND(rdst, rsrc) this->writer->and_(rdst, rsrc);
#define WRITER_OR(rdst, rsrc) this->writer->or_(rdst, rsrc);
#define WRITER_XOR(rdst, rsrc) this->writer->xor_(rdst, rsrc);
#define WRITER_SHL(rdst, rsrc) this->writer->shl(rdst, rsrc);
#define WRITER_SHR(rdst, rsrc) this->writer->shr(rdst, rsrc);
#define WRITER_SAL(rdst, rsrc) this->writer->sal(rdst, rsrc);
#define WRITER_SAR(rdst, rsrc) this->writer->sar(rdst, rsrc);
#define WRITER_CMP(rdst, rsrc) this->writer->cmp(rdst, rsrc);

#define WRITER_JBOUNDS() \
    this->writer->jge(GetHandlerLabel("bounds"));

#define WRITER_JGUARD() \
    this->writer->jz(GetHandlerLabel("guard"));

#endif