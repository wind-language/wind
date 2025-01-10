#ifndef BACKEND_EXPR_MACROS
#define BACKEND_EXPR_MACROS

#define CASTED_MOV(dst, src) \
    if (dst.size > src.size || dst.size < src.size) { \
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


#define VA_ASSIGN_OP(src) \
    const IRLocalAddrRef *ref = binop->left()->as<IRLocalAddrRef>(); \
    if (ref->isIndexed()) { \
        if (!ref->datatype()->isArray()) { \
            throw std::runtime_error("Cannot index non-array"); \
        } \
        if (ref->getIndex()->type() == IRNode::NodeType::LITERAL) { \
            uint16_t offset = ref->offset() - ref->datatype()->index2offset(ref->getIndex()->as<IRLiteral>()->get()); \
            if (offset < 0) { \
                throw std::runtime_error("Invalid offset"); \
            } \
            src = this->CastReg(src, ref->datatype()->rawSize()); \
            this->writer->mov( \
                this->writer->ptr( \
                    x86::Gp::rbp, \
                    -offset, \
                    ref->datatype()->rawSize() \
                ), \
                src \
            ); \
        } \
        else { \
            Reg r_index = this->EmitValue(ref->getIndex(), dst); \
            Reg index = {r_index.id, 8, Reg::GPR, false}; \
            this->TryCast(index, r_index); \
            src = this->CastReg(src, ref->datatype()->rawSize()); \
            this->writer->mov( \
                this->writer->ptr( \
                    x86::Gp::rbp, \
                    index, \
                    -ref->offset(), \
                    ref->datatype()->rawSize() \
                ), \
                src \
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
            uint16_t offset = ref->offset() - ref->datatype()->index2offset(ref->getIndex()->as<IRLiteral>()->get()); \
            if (offset < 0) { \
                throw std::runtime_error("Invalid offset"); \
            } \
            src = this->CastReg(src, ref->datatype()->rawSize()); \
            this->writer->op( \
                this->writer->ptr( \
                    x86::Gp::rbp, \
                    -offset, \
                    ref->datatype()->rawSize() \
                ), \
                src \
            ); \
            this->writer->mov( \
                this->writer->ptr( \
                    x86::Gp::rbp, \
                    -offset, \
                    ref->datatype()->rawSize() \
                ), \
                src \
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
                    -ref->offset(), \
                    ref->datatype()->rawSize() \
                ), \
                src \
            ); \
            this->writer->mov( \
                this->writer->ptr( \
                    x86::Gp::rbp, \
                    index, \
                    -ref->offset(), \
                    ref->datatype()->rawSize() \
                ), \
                src \
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
            uint16_t offset = ref->offset() - ref->datatype()->index2offset(ref->getIndex()->as<IRLiteral>()->get()); \
            if (offset < 0) { \
                throw std::runtime_error("Invalid offset"); \
            } \
            this->writer->mov( \
                this->writer->ptr( \
                    x86::Gp::rbp, \
                    -offset, \
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
                    -ref->offset(), \
                    ref->datatype()->rawSize() \
                ), \
                src \
            ); \
        } \
    }

#define RAW_VALIT_OPASSIGN(src, op) \
    const IRLocalAddrRef *ref = binop->left()->as<IRLocalAddrRef>(); \
    if (ref->isIndexed()) { \
        if (!ref->datatype()->isArray()) { \
            throw std::runtime_error("Cannot index non-array"); \
        } \
        if (ref->getIndex()->type() == IRNode::NodeType::LITERAL) { \
            uint16_t offset = ref->offset() - ref->datatype()->index2offset(ref->getIndex()->as<IRLiteral>()->get()); \
            if (offset < 0) { \
                throw std::runtime_error("Invalid offset"); \
            } \
            this->writer->op( \
                this->writer->ptr( \
                    x86::Gp::rbp, \
                    -offset, \
                    ref->datatype()->rawSize() \
                ), \
                src \
            ); \
            this->writer->mov( \
                this->writer->ptr( \
                    x86::Gp::rbp, \
                    -offset, \
                    ref->datatype()->rawSize() \
                ), \
                src \
            ); \
        } \
        else { \
            Reg r_index = this->EmitValue(ref->getIndex(), dst); \
            Reg index = {r_index.id, 8, Reg::GPR, false}; \
            this->TryCast(index, r_index); \
            this->writer->op( \
                this->writer->ptr( \
                    x86::Gp::rbp, \
                    index, \
                    -ref->offset(), \
                    ref->datatype()->rawSize() \
                ), \
                src \
            ); \
            this->writer->mov( \
                this->writer->ptr( \
                    x86::Gp::rbp, \
                    index, \
                    -ref->offset(), \
                    ref->datatype()->rawSize() \
                ), \
                src \
            ); \
        } \
    } \

#define LIT_VA_ASSIGN(type) \
    case type: { \
        RAW_VALIT_ASSIGN(binop->right()->as<IRLiteral>()->get()); \
        return dst; \
    }

#define LIT_VA_OPASSIGN(type, op) \
    case type: { \
        RAW_VALIT_OPASSIGN(binop->right()->as<IRLiteral>()->get(), op); \
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
    CASTED_MOV(dst, this->writer->ptr( \
        x86::Gp::rbp, \
        -binop->left()->as<IRLocalRef>()->offset(), \
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

#endif