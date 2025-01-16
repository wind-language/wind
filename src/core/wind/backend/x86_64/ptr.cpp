#include <wind/generation/IR.h>
#include <wind/backend/writer/writer.h>
#include <wind/backend/x86_64/backend.h>
#include <wind/backend/x86_64/expr_macros.h>
#include <wind/bridge/opt_flags.h>
#include <stdexcept>
#include <iostream>

Reg WindEmitter::EmitLocAddrRef(IRLocalAddrRef *ref, Reg dst) {
    if (ref->isIndexed()) {
        if (!ref->datatype()->isArray()) { // pointer indexing
            CASTED_MOV(
                CastReg(dst, 8),
                this->writer->ptr(
                    x86::Gp::rbp,
                    ref->offset(),
                    ref->datatype()->moveSize()
                )
            );
            IRNode *index = ref->getIndex();
            if (index->is<IRLiteral>()) {
                CASTED_MOV(
                    dst,
                    this->writer->ptr(
                        CastReg(dst, 8),
                        ref->datatype()->index2offset(index->as<IRLiteral>()->get()),
                        ref->datatype()->rawSize()
                    )
                )
            } else {
                Reg r_index = this->EmitExpr(index, x86::Gp::rbx);
                CASTED_MOV(
                    dst,
                    this->writer->ptr(
                        CastReg(dst, 8),
                        CastReg(r_index, 8),
                        0,
                        ref->datatype()->rawSize()
                    )
                )
            }
            return dst;
        }
        if (ref->getIndex()->type() == IRNode::NodeType::LITERAL) {
            int16_t offset = ref->offset() - ref->datatype()->index2offset(ref->getIndex()->as<IRLiteral>()->get());
            if (offset < 0) {
                throw std::runtime_error("Invalid offset");
            }
            dst.size = ref->datatype()->moveSize(); dst.signed_value = ref->datatype()->isSigned();
            CASTED_MOV(
                dst,
                this->writer->ptr(
                    x86::Gp::rbp,
                    offset,
                    ref->datatype()->rawSize()
                )
            );
        } else {
            Reg r_index = this->EmitExpr(ref->getIndex(), dst);
            Reg index = {r_index.id, 8, Reg::GPR, false};
            this->TryCast(index, r_index);
            regalloc.SetDirty(index);
            if (!ref->datatype()->hasCapacity() || this->current_fn->fn->flags & PURE_STCHK) {
                CASTED_MOV(
                    dst,
                    this->writer->ptr(
                        x86::Gp::rbp,
                        index,
                        ref->offset(),
                        ref->datatype()->rawSize()
                    )
                );
            } else {
                Reg addr_holder = this->regalloc.Allocate(8);
                this->writer->OverflowChecks = false;
                this->writer->lea(
                    addr_holder,
                    this->writer->ptr(
                        x86::Gp::rbp,
                        index,
                        ref->offset(),
                        ref->datatype()->rawSize()
                    )
                );
                this->writer->sub(
                    addr_holder,
                    x86::Gp::rbp
                );
                this->writer->cmp(
                    addr_holder,
                    -ref->offset()+ref->datatype()->memSize()
                );
                WRITER_JBOUNDS();
                this->writer->add(
                    addr_holder,
                    x86::Gp::rbp
                );
                CASTED_MOV(
                    dst,
                    this->writer->ptr(
                        addr_holder,
                        0,
                        ref->datatype()->rawSize()
                    )
                );
                this->writer->OverflowChecks = true;
                this->regalloc.Free(addr_holder);
            }
            this->regalloc.Free(index);
        }
    } else {
        this->writer->lea(
            dst,
            this->writer->ptr(
                x86::Gp::rbp,
                ref->offset(),
                8
            )
        );
    }
    return {dst.id, 8, Reg::GPR, false};
}

void WindEmitter::EmitIntoLocAddrRef(IRLocalAddrRef *ref, Reg src) {
    if (ref->isIndexed()) {
        if (!ref->datatype()->isArray()) {
            // pointer indexing
            src.size = ref->datatype()->rawSize(); src.signed_value = ref->datatype()->isSigned();
            CASTED_MOV(
                x86::Gp::rbx,
                this->writer->ptr(
                    x86::Gp::rbp,
                    ref->offset(),
                    ref->datatype()->moveSize()
                )
            )
            this->regalloc.SetVar(x86::Gp::rbx, ref->offset(), RegisterAllocator::RegValue::Lifetime::UNTIL_ALLOC);
            if (ref->getIndex()->is<IRLiteral>()) {
                int16_t index = ref->getIndex()->as<IRLiteral>()->get();
                if (index < 0) {
                    throw std::runtime_error("Invalid offset");
                }
                this->writer->mov(
                    this->writer->ptr(
                        x86::Gp::rbx,
                        ref->datatype()->index2offset(index),
                        src.size
                    ),
                    src
                );
            } else {
                this->regalloc.SetDirty(src); // Keep src alive
                Reg r_index = this->EmitExpr(ref->getIndex(), this->regalloc.Allocate(8, false));
                Reg index = {r_index.id, 8, Reg::GPR, false};
                this->TryCast(index, r_index);
                this->writer->mov(
                    this->writer->ptr(
                        x86::Gp::rbx,
                        index,
                        0,
                        src.size
                    ),
                    src
                );
                this->regalloc.Free(index);
            }
            return;
        }
    } else {
        // &var = src ;; doesn't have any use case
        return;
    }

    // array indexing
    if (ref->getIndex()->is<IRLiteral>()) {
        int16_t offset = ref->offset() - ref->datatype()->index2offset(ref->getIndex()->as<IRLiteral>()->get());
        if (offset < 0) {
            throw std::runtime_error("Invalid offset");
        }
        src.size = ref->datatype()->moveSize(); src.signed_value = ref->datatype()->isSigned();
        this->writer->mov(
            this->writer->ptr(
                x86::Gp::rbp,
                offset,
                ref->datatype()->rawSize()
            ),
            src
        );
    } else {
        this->regalloc.SetDirty(src); // Keep src alive
        Reg r_index = this->EmitExpr(ref->getIndex(), src);
        Reg index = {r_index.id, 8, Reg::GPR, false};
        this->TryCast(index, r_index);
        regalloc.SetDirty(index);
        if (!ref->datatype()->hasCapacity() || this->current_fn->fn->flags & PURE_STCHK) {
            this->writer->mov(
                this->writer->ptr(
                    x86::Gp::rbp,
                    index,
                    ref->offset(),
                    ref->datatype()->rawSize()
                ),
                src
            );
        } else {
            Reg addr_holder = this->regalloc.Allocate(8);
            this->writer->OverflowChecks = false;
            this->writer->lea(
                addr_holder,
                this->writer->ptr(
                    x86::Gp::rbp,
                    index,
                    ref->offset(),
                    ref->datatype()->rawSize()
                )
            );
            this->writer->sub(
                addr_holder,
                x86::Gp::rbp
            );
            this->writer->cmp(
                addr_holder,
                -ref->offset()+ref->datatype()->memSize()
            );
            WRITER_JBOUNDS();
            this->writer->add(
                addr_holder,
                x86::Gp::rbp
            );
            this->writer->mov(
                this->writer->ptr(
                    addr_holder,
                    0,
                    ref->datatype()->rawSize()
                ),
                CastReg(src, ref->datatype()->rawSize())
            );
            this->writer->OverflowChecks = true;
            this->regalloc.Free(addr_holder);
        }
        this->regalloc.Free(index);
    }
}

Reg WindEmitter::EmitFnRef(IRFnRef *ref, Reg dst) {
    this->writer->lea(
        dst,
        this->writer->ptr(
            ref->name(),
            0,
            8
        )
    );
    return {dst.id, 8, Reg::GPR, false};
}

Reg WindEmitter::EmitGenAddrRef(IRGenericIndexing *ref, Reg dst) {
    IRNode *index = ref->getIndex();
    if (index->is<IRLiteral>()) {
        Reg base = this->EmitExpr(ref->getBase(), x86::Gp::rbx);
        int16_t indexv = index->as<IRLiteral>()->get();
        CASTED_MOV(
            dst,
            this->writer->ptr(
                CastReg(base, 8),
                ref->inferType()->index2offset(indexv),
                ref->inferType()->rawSize()
            )
        )
        return dst;
    }
    Reg base = this->EmitExpr(ref->getBase(), x86::Gp::rbx);
    Reg r_index = this->EmitExpr(index, this->regalloc.Allocate(8, false));
    r_index = {r_index.id, 8, Reg::GPR, false};
    CASTED_MOV(
        dst,
        this->writer->ptr(
            CastReg(base, 8),
            CastReg(r_index, 8),
            0,
            ref->inferType()->rawSize()
        )
    )
    return dst;
}

Reg WindEmitter::EmitPtrGuard(IRPtrGuard *ref, Reg dst) {
    Reg ptr = this->EmitExpr(ref->getValue(), dst);
    this->writer->test(ptr, ptr);
    WRITER_JGUARD();
    return ptr;
}

Reg WindEmitter::EmitTypeCast(IRTypeCast *cast, Reg dst) {
    Reg val = this->EmitExpr(cast->getValue(), dst);
    if (val.size == cast->getType()->rawSize()) {
        return val;
    }
    if (val.size < cast->getType()->rawSize()) {
        if (val.signed_value) {
            this->writer->movsx(CastReg(val, cast->getType()->rawSize()), val);
        } else {
            this->writer->movzx(CastReg(val, cast->getType()->rawSize()), val);
        }
    }
    return CastReg(val, cast->getType()->rawSize());
}

void WindEmitter::EmitIntoGenAddrRef(IRGenericIndexing *ref, Reg src) {
    IRNode *index = ref->getIndex();
    if (index->is<IRLiteral>()) {
        Reg base = this->EmitExpr(ref->getBase(), x86::Gp::rbx);
        int16_t indexv = index->as<IRLiteral>()->get();
        this->writer->mov(
            this->writer->ptr(
                CastReg(base, 8),
                ref->inferType()->index2offset(indexv),
                ref->inferType()->rawSize()
            ),
            CastReg(src, ref->inferType()->rawSize())
        );
        return;
    }

    this->regalloc.SetDirty(src); // Keep src alive
    Reg base = this->EmitExpr(ref->getBase(), x86::Gp::rbx);
    Reg r_index = this->EmitExpr(index, src);
    r_index = {r_index.id, 8, Reg::GPR, false};
    this->writer->mov(
        this->writer->ptr(
            CastReg(base, 8),
            CastReg(r_index, 8),
            0,
            ref->inferType()->rawSize()
        ),
        CastReg(src, ref->inferType()->rawSize())
    );
}