#include <wind/generation/IR.h>
#include <wind/backend/writer/writer.h>
#include <wind/backend/x86_64/backend.h>
#include <wind/bridge/flags.h>
#include <stdexcept>
#include <iostream>

Reg WindEmitter::EmitValLiteral(IRLiteral *lit, Reg dst) {
    int64_t val = lit->get();
    if (val==0) {
        this->writer->xor_(dst, dst);
    } else {
        this->writer->mov(dst, val);
    }
    return dst;
}

Reg WindEmitter::EmitStrLiteral(IRStringLiteral *lit, Reg dst) {
    this->writer->lea(
        dst,
        this->writer->ptr(
            this->state->string(lit->get()),
            0,
            8
        )
    );
    return dst;
}

Reg WindEmitter::EmitTypeCast(IRTypeCast *cast, Reg dst) {
    Reg src = this->EmitExpr(cast->getValue(), dst, false);
    this->TryCast(dst, src);
    return dst;
}

Reg WindEmitter::EmitValue(IRNode *node, Reg dst) {
    switch (node->type()) {
        case IRNode::NodeType::LITERAL: return this->EmitValLiteral(node->as<IRLiteral>(), dst);
        case IRNode::NodeType::STRING: return this->EmitStrLiteral(node->as<IRStringLiteral>(), dst);
        case IRNode::NodeType::LOCAL_REF: return this->EmitLocalVar(node->as<IRLocalRef>(), dst);
        case IRNode::NodeType::GLOBAL_REF: return this->EmitGlobalVar(node->as<IRGlobRef>(), dst);
        case IRNode::NodeType::FUNCTION_CALL: return this->EmitFnCall(node->as<IRFnCall>(), dst);
        case IRNode::NodeType::TYPE_CAST: return this->EmitTypeCast(node->as<IRTypeCast>(), dst);
        case IRNode::NodeType::LADDR_REF: return this->EmitLocalPtr(node->as<IRLocalAddrRef>(), dst);
        case IRNode::NodeType::GENERIC_INDEXING: return this->EmitGenPtrIndex(node->as<IRGenericIndexing>(), dst);
        case IRNode::NodeType::PTR_GUARD: return this->EmitPtrGuard(node->as<IRPtrGuard>(), dst);
        case IRNode::NodeType::STRUCT_VAL: {
            throw std::runtime_error("Non-local Static struct value not managed yet");
        }
        case IRNode::NodeType::LOC_STRUCT_ACC: return this->EmitLocFieldAcc(node->as<IRLocFieldAccess>(), dst);
        default: {
            throw std::runtime_error("Invalid value node type(" + std::to_string((uint8_t)node->type()) + "): Report to maintainer!");
        }
    }
}

template <typename srcT>
Reg WindEmitter::EmitArithmeticOp(IRBinOp::Operation op, Reg dst, srcT right, bool isJmp) {
    if (this->state->jmp_map.find(op) != this->state->jmp_map.end()) {
        if (op == IRBinOp::Operation::LOGAND) this->writer->test(dst, right);
        else this->writer->cmp(dst, right);
        
        if (isJmp) return dst;
    }
    switch (op) {
        case IRBinOp::Operation::ADD: {
            this->writer->add(dst, right, this->state->RequestHandler(HandlerType::SUM_HANDLER));
            break;
        }
        case IRBinOp::Operation::SUB: {
            this->writer->sub(dst, right, this->state->RequestHandler(HandlerType::SUB_HANDLER));
            break;
        }
        case IRBinOp::Operation::AND: {
            this->writer->and_(dst, right);
            break;
        }
        case IRBinOp::Operation::OR: {
            this->writer->or_(dst, right);
            break;
        }
        case IRBinOp::Operation::LESS: {
            dst = CastReg(dst, 1);
            if (dst.signed_value) this->writer->setl(dst);
            else this->writer->setb(dst);
            break;
        }
        case IRBinOp::Operation::EQ: {
            dst = CastReg(dst, 1);
            this->writer->sete(dst);
            break;
        }
        case IRBinOp::Operation::NOTEQ: {
            dst = CastReg(dst, 1);
            this->writer->setne(dst);
            break;
        }
        case IRBinOp::Operation::GREATER: {
            dst = CastReg(dst, 1);
            if (dst.signed_value) this->writer->setg(dst);
            else this->writer->seta(dst);
            break;
        }
        case IRBinOp::Operation::LESSEQ: {
            dst = CastReg(dst, 1);
            if (dst.signed_value) this->writer->setle(dst);
            else this->writer->setbe(dst);
            break;
        }
        case IRBinOp::Operation::GREATEREQ: {
            dst = CastReg(dst, 1);
            if (dst.signed_value) this->writer->setge(dst);
            else this->writer->setae(dst);
            break;
        }
        case IRBinOp::Operation::LOGAND: {
            this->writer->test(dst, right);
            this->writer->setnz(dst);
            break;
        }
        case IRBinOp::Operation::SHL: {
            if (dst.signed_value) this->writer->sal(dst, right);
            else this->writer->shl(dst, right);
            break;
        }
        case IRBinOp::Operation::SHR: {
            if (dst.signed_value) this->writer->sar(dst, right);
            else this->writer->shr(dst, right);
            break;
        }
        case IRBinOp::Operation::XOR: {
            this->writer->xor_(dst, right);
            break;
        }
        case IRBinOp::Operation::DIV: {
            bool rdx_restore = this->regalloc->isDirty(x86::Gp::rdx);
            RegisterAllocator::RegValue rdx = this->regalloc->GetRegState(x86::Gp::rdx);
            if (rdx_restore) {
                this->writer->push(x86::Gp::rdx);
            }
            TryCast(x86::Gp::rax, dst);
            this->writer->xor_(x86::Gp::rdx, x86::Gp::rdx);
            if (dst.signed_value) this->writer->cqo();

            if (typeid(srcT) != typeid(Reg)) {
                Reg src = this->regalloc->Allocate(8, true);
                this->writer->mov(src, right);
                if (dst.signed_value) this->writer->idiv(src, this->state->RequestHandler(HandlerType::DIV_HANDLER));
                else this->writer->div(src, this->state->RequestHandler(HandlerType::DIV_HANDLER));
            } else {
                if (dst.signed_value) this->writer->idiv(right, this->state->RequestHandler(HandlerType::DIV_HANDLER));
                else this->writer->div(right, this->state->RequestHandler(HandlerType::DIV_HANDLER));
            }

            this->regalloc->Free(x86::Gp::rdx);
            this->regalloc->Free(x86::Gp::rax);
            if (rdx_restore) {
                this->writer->pop(x86::Gp::rdx);
                this->regalloc->SetRegState(x86::Gp::rdx, rdx);
            }
            return CastReg(x86::Gp::rax, 8);
        }
        case IRBinOp::Operation::MOD: {
            bool rdx_restore = this->regalloc->isDirty(x86::Gp::rdx);
            RegisterAllocator::RegValue rdx = this->regalloc->GetRegState(x86::Gp::rdx);
            if (rdx_restore) {
                this->writer->push(x86::Gp::rdx);
            }
            TryCast(x86::Gp::rax, dst);
            this->writer->xor_(x86::Gp::rdx, x86::Gp::rdx);
            if (dst.signed_value) this->writer->cqo();

            if (typeid(srcT) != typeid(Reg)) {
                Reg src = this->regalloc->Allocate(8, true);
                this->writer->mov(src, right);
                if (dst.signed_value) this->writer->idiv(src, this->state->RequestHandler(HandlerType::DIV_HANDLER));
                else this->writer->div(src, this->state->RequestHandler(HandlerType::DIV_HANDLER));
            } else {
                if (dst.signed_value) this->writer->idiv(right, this->state->RequestHandler(HandlerType::DIV_HANDLER));
                else this->writer->div(right, this->state->RequestHandler(HandlerType::DIV_HANDLER));
            }

            this->regalloc->Free(x86::Gp::rdx);
            this->regalloc->Free(x86::Gp::rax);
            if (rdx_restore) {
                this->writer->pop(x86::Gp::rdx);
                this->regalloc->SetRegState(x86::Gp::rdx, rdx);
            }
            return CastReg(x86::Gp::rdx, 8);
        }
        case IRBinOp::Operation::MUL: {
            if (dst.signed_value) {
                this->writer->imul(dst, right);
                break;
            }
            if (typeid(srcT) != typeid(Reg)) {
                Reg src = this->regalloc->Allocate(8, true);
                this->writer->mov(src, right);
                this->writer->mul(src, this->state->RequestHandler(HandlerType::MUL_HANDLER));
            } else {
                this->writer->mul(right, this->state->RequestHandler(HandlerType::MUL_HANDLER));
            }
            break;
        }
        default: {
            throw std::runtime_error("Invalid operation type(" + std::to_string((uint8_t)op) + "): Report to maintainer!");
        }
    }
    return dst;
}

Reg WindEmitter::EmitOptArithmeticLVOp(IRBinOp::Operation op, Reg dst, const IRLocalRef *local, bool isJmp) {
    Reg *freg = this->regalloc->FindLocalVar(local->offset(), local->datatype()->moveSize());
    if (freg) {
        return this->EmitArithmeticOp(op, dst, *freg, isJmp);
    }
    return this->EmitArithmeticOp(
        op,
        dst,
        this->writer->ptr(
            x86::Gp::rbp,
            local->offset(),
            local->datatype()->moveSize()
        ),
        isJmp
    );
}

Reg WindEmitter::EmitOptArithmeticGVOp(IRBinOp::Operation op, Reg dst, const IRGlobRef *global, bool isJmp) {
    Reg *freg = this->regalloc->FindLabel(global->getName(), global->getType()->moveSize());
    if (freg) {
        return this->EmitArithmeticOp(op, dst, *freg, isJmp);
    }
    return this->EmitArithmeticOp(
        op,
        dst,
        this->writer->ptr(
            global->getName(),
            0,
            global->getType()->moveSize()
        ),
        isJmp
    );
}


Reg WindEmitter::EmitBinOp(IRBinOp *expr, Reg dst, bool isJmp) {
    switch (expr->operation()) {
        case IRBinOp::Operation::L_ASSIGN:
            this->EmitIntoLocalVar((IRLocalRef*)expr->left()->as<IRLocalRef>(), (IRNode*)expr->right());
            return dst;
        case IRBinOp::Operation::G_ASSIGN:
            this->EmitIntoGlobalVar((IRGlobRef*)expr->left()->as<IRGlobRef>(), (IRNode*)expr->right());
            return dst;
        case IRBinOp::Operation::GEN_INDEX_ASSIGN:
            this->EmitIntoGenPtr(expr->left(), expr->right());
            return dst;
        case IRBinOp::Operation::LOC_STRUCT_ASSIGN:
            this->EmitStructIntoLocal((IRLocalRef*)expr->left()->as<IRLocalRef>(), (IRStructValue*)expr->right()->as<IRStructValue>());
            return dst;
        default: {}
    }
    dst = this->EmitExpr((IRNode*)expr->left(), dst, false);
    this->regalloc->SetDirty(dst);
    Reg right;
    switch (expr->right()->type()) {
        case IRNode::NodeType::LITERAL: return this->EmitArithmeticOp(expr->operation(), dst, expr->right()->as<IRLiteral>()->get(), isJmp);
        case IRNode::NodeType::LOCAL_REF: return this->EmitOptArithmeticLVOp(expr->operation(), dst, expr->right()->as<IRLocalRef>(), isJmp);
        case IRNode::NodeType::GLOBAL_REF: return this->EmitOptArithmeticGVOp(expr->operation(), dst, expr->right()->as<IRGlobRef>(), isJmp);
        default: {
            right = this->EmitExpr((IRNode*)expr->right(), this->regalloc->Allocate(dst.size, true), false);
        }
    }

    EmitArithmeticOp(expr->operation(), dst, right, isJmp);

    return dst;
}

void WindEmitter::TryCast(Reg dst, Reg src) {
    if (dst.id==src.id && dst.size==8 && src.size==4) {
        if (!dst.signed_value) {return;}
        if (dst.id==x86::Gp::rax.id) {
            this->writer->cdqe();
        } else {
            this->writer->movsx(dst, src);
        }
    }
    else if (dst.size > src.size) {
        if (dst.signed_value) {
            this->writer->movsx(dst, src);
        } else {
            this->writer->movzx(dst, src);
        }
    }
    else if (dst.id != src.id) {
        this->writer->mov(dst, src);
    }
}

Reg WindEmitter::EmitExpr(IRNode *node, Reg dst, bool keepDstSize, bool isJmp) {
    Reg res = dst;
    if (node->is<IRBinOp>()) res = this->EmitBinOp(node->as<IRBinOp>(), dst, isJmp);
    else res = this->EmitValue(node, dst);


    if (keepDstSize) { this->TryCast(dst, res); res.id = dst.id; res.size = dst.size; }
    this->regalloc->PostExpression();
    return res;
}