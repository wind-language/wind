#include <wind/generation/IR.h>
#include <wind/backend/writer/writer.h>
#include <wind/backend/x86_64/backend.h>
#include <wind/backend/x86_64/expr_macros.h>
#include <wind/bridge/opt_flags.h>
#include <stdexcept>

Reg WindEmitter::EmitLocAddrRef(IRLocalAddrRef *ref, Reg dst) {
    if (ref->isIndexed()) {
        if (!ref->datatype()->isArray()) {
            // TOOD: Implement data type indexing for pointers
            // byte for now
            CASTED_MOV(
                dst,
                this->writer->ptr(
                    x86::Gp::rbp,
                    -ref->offset(),
                    ref->datatype()->rawSize()
                )
            )
            if (ref->getIndex()->type() == IRNode::NodeType::LITERAL) {
                int16_t index = ref->getIndex()->as<IRLiteral>()->get();
                if (index < 0) {
                    throw std::runtime_error("Invalid offset");
                }
                CASTED_MOV(
                    dst,
                    this->writer->ptr(
                        dst,
                        index,
                        1 // byte type
                    )
                );
            } else {
                this->regalloc.SetDirty(dst);
                Reg r_index = this->EmitExpr(ref->getIndex(), this->regalloc.Allocate(8, false));
                Reg index = {r_index.id, 8, Reg::GPR, false};
                this->TryCast(index, r_index);
                CASTED_MOV(
                    dst,
                    this->writer->ptr(
                        dst,
                        index,
                        0,
                        1 // byte type
                    )
                );
                this->regalloc.Free(index);
            }
            return {dst.id, 1, Reg::GPR, false};
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
                    -offset,
                    ref->datatype()->rawSize()
                )
            );
        } else {
            Reg r_index = this->EmitExpr(ref->getIndex(), dst);
            Reg index = {r_index.id, 8, Reg::GPR, false};
            this->TryCast(index, r_index);
            regalloc.SetDirty(index);
            if (!ref->datatype()->hasCapacity() || this->current_fn->flags & PURE_STCHK) {
                CASTED_MOV(
                    dst,
                    this->writer->ptr(
                        x86::Gp::rbp,
                        index,
                        -ref->offset(),
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
                        -ref->offset(),
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
                this->writer->jge("__WDH_out_of_bounds");
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
                -ref->offset(),
                8
            )
        );
    }
    return {dst.id, 8, Reg::GPR, false};
}

void WindEmitter::EmitIntoLocAddrRef(IRLocalAddrRef *ref, Reg src) {
    if (ref->isIndexed()) {
        if (!ref->datatype()->isArray()) {
            // TOOD: Implement data type indexing for pointers
            // byte for now
            src.size = 1;
            CASTED_MOV(
                x86::Gp::rbx,
                this->writer->ptr(
                    x86::Gp::rbp,
                    -ref->offset(),
                    ref->datatype()->rawSize()
                )
            )
            if (ref->getIndex()->is<IRLiteral>()) {
                int16_t index = ref->getIndex()->as<IRLiteral>()->get();
                if (index < 0) {
                    throw std::runtime_error("Invalid offset");
                }
                this->writer->mov(
                    this->writer->ptr(
                        x86::Gp::rbx,
                        index,
                        1 // byte type
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
                        1 // byte type
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
        CASTED_MOV(
            src,
            this->writer->ptr(
                x86::Gp::rbp,
                -offset,
                ref->datatype()->rawSize()
            )
        );
    } else {
        Reg r_index = this->EmitExpr(ref->getIndex(), src);
        Reg index = {r_index.id, 8, Reg::GPR, false};
        this->TryCast(index, r_index);
        regalloc.SetDirty(index);
        if (!ref->datatype()->hasCapacity() || this->current_fn->flags & PURE_STCHK) {
            CASTED_MOV(
                src,
                this->writer->ptr(
                    x86::Gp::rbp,
                    index,
                    -ref->offset(),
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
                    -ref->offset(),
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
            this->writer->jge("__WDH_out_of_bounds");
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