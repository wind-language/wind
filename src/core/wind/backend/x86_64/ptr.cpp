#include <wind/generation/IR.h>
#include <wind/backend/writer/writer.h>
#include <wind/backend/x86_64/backend.h>
#include <wind/bridge/flags.h>
#include <stdexcept>
#include <iostream>

Reg WindEmitter::EmitLocalPtr(IRLocalAddrRef *local, Reg dst) {
    if (local->datatype()->isArray() && local->isIndexed()) {
        // TODO: Implement array indexing
    }
    else {
        // Load the address of the local variable
        this->writer->lea(
            dst,
            this->writer->ptr(
                x86::Gp::rbp,
                local->offset(),
                local->datatype()->moveSize()
            )
        );
    }
    return CastReg(dst, 8);
}

void WindEmitter::EmitIntoGenPtr(const IRNode *ptr, const IRNode *value) {
    if (ptr->type() == IRNode::NodeType::LADDR_REF) {
        throw std::runtime_error("TODO: Implement array indexing");
    }

    const IRGenericIndexing *ndexing = ptr->as<IRGenericIndexing>();
    Reg src = this->regalloc->Allocate(ndexing->inferType()->rawSize(), false);
    if (value->type() != IRNode::NodeType::LITERAL) {
        src = this->EmitExpr((IRNode*)value, src);
    }
    Reg base = this->EmitExpr(ndexing->getBase(), this->regalloc->Allocate(8, false));
    this->regalloc->SetIndexing(base);
    TryCast(CastReg(base, 8), base);
    base = CastReg(base, 8);

    IRNode *index = ndexing->getIndex();
    Mem dst_ptr = this->writer->ptr(x86::Gp::rax, 0, 8); // placeholder
    Reg c_index;

    switch (index->type()) {
        case IRNode::NodeType::LITERAL: {
            dst_ptr = this->writer->ptr(
                base,
                ndexing->inferType()->index2offset(index->as<IRLiteral>()->get()),
                ndexing->inferType()->rawSize()
            );
            break;
        }
        default: {
            Reg r_index = this->EmitExpr(ndexing->getIndex(), this->regalloc->Allocate(8, false));
            this->regalloc->SetIndexing(r_index);
            c_index = {r_index.id, 8, Reg::GPR, false};
            TryCast(c_index, r_index);
            dst_ptr = this->writer->ptr(
                base,
                c_index,
                0,
                ndexing->inferType()->rawSize()
            );
        }
    }
    switch (value->type()) {
        case IRNode::NodeType::LITERAL: {
            this->writer->mov(
                dst_ptr,
                value->as<IRLiteral>()->get()
            );
            break;
        }
        default: {
            TryCast(CastReg(src, ndexing->inferType()->rawSize()), src);
            this->writer->mov(
                dst_ptr,
                CastReg(src, ndexing->inferType()->rawSize())
            );
            this->regalloc->Free(src);
        }
    }
    if (index->type() != IRNode::NodeType::LITERAL) {
        this->regalloc->Free(c_index);
    }
    this->regalloc->Free(src);
    this->regalloc->Free(base);
}

Reg WindEmitter::EmitGenPtrIndex(IRGenericIndexing *ptr, Reg dst) {
    if (ptr->type() == IRNode::NodeType::LADDR_REF) {
        throw std::runtime_error("TODO: Implement array indexing");
    }

    const IRGenericIndexing *ndexing = ptr->as<IRGenericIndexing>();
    Reg base = this->EmitExpr(ndexing->getBase(), this->regalloc->Allocate(8, false));
    this->regalloc->SetIndexing(base);
    TryCast(CastReg(base, 8), base);
    base = CastReg(base, 8);

    IRNode *index = ndexing->getIndex();
    Mem dst_ptr = this->writer->ptr(x86::Gp::rax, 0, 8); // placeholder
    Reg c_index;

    switch (index->type()) {
        case IRNode::NodeType::LITERAL: {
            dst_ptr = this->writer->ptr(
                base,
                ndexing->inferType()->index2offset(index->as<IRLiteral>()->get()),
                ndexing->inferType()->rawSize()
            );
            break;
        }
        default: {
            Reg r_index = this->EmitExpr(ndexing->getIndex(), this->regalloc->Allocate(8, false));
            this->regalloc->SetIndexing(r_index);
            c_index = {r_index.id, 8, Reg::GPR, false};
            TryCast(c_index, r_index);
            dst_ptr = this->writer->ptr(
                base,
                c_index,
                0,
                ndexing->inferType()->rawSize()
            );
        }
    }
    dst.size = ndexing->inferType()->rawSize();
    this->writer->mov(
        dst,
        dst_ptr
    );
    if (index->type() != IRNode::NodeType::LITERAL) {
        this->regalloc->Free(c_index);
    }
    this->regalloc->Free(base);
    return dst;
}