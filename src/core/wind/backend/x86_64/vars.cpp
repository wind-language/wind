#include <wind/generation/IR.h>
#include <wind/backend/writer/writer.h>
#include <wind/backend/x86_64/backend.h>
#include <wind/bridge/flags.h>
#include <iostream>

Reg WindEmitter::EmitLocalVar(IRLocalRef *local, Reg dst) {
    dst.size = local->datatype()->moveSize();
    Reg *reg = this->regalloc->FindLocalVar(local->offset(), local->datatype()->moveSize());
    if (reg) {
        if (reg->id != dst.id) TryCast(dst, *reg);
    } else {
        this->writer->mov(
            dst,
            this->writer->ptr(
                x86::Gp::rbp,
                local->offset(),
                local->datatype()->moveSize()
            )
        );
        this->regalloc->SetVar(dst, local->offset(), RegisterAllocator::RegValue::Lifetime::UNTIL_ALLOC);
    }
    dst.signed_value = local->datatype()->isSigned();
    return dst;
}

Reg WindEmitter::EmitGlobalVar(IRGlobRef *global, Reg dst) {
    dst.size = global->getType()->moveSize();
    Reg *reg = this->regalloc->FindLabel(global->getName(), global->getType()->moveSize());
    if (reg) {
        if (reg->id != dst.id) TryCast(dst, *reg);
    } else {
        this->writer->mov(
            dst,
            this->writer->ptr(
                global->getName(),
                0,
                global->getType()->moveSize()
            )
        );
        this->regalloc->SetLabel(dst, global->getName(), RegisterAllocator::RegValue::Lifetime::UNTIL_ALLOC);
    }
    dst.signed_value = global->getType()->isSigned();
    return dst;
}

void WindEmitter::EmitIntoLocalVar(IRLocalRef *local, IRNode *value) {
    switch (value->type()) {
        case IRNode::NodeType::LITERAL: {        
            this->writer->mov(
                this->writer->ptr(
                    x86::Gp::rbp,
                    local->offset(),
                    local->datatype()->moveSize()
                ),
                value->as<IRLiteral>()->get()
            );
            break;
        }
        case IRNode::NodeType::LOCAL_REF: {
            Reg *freg = this->regalloc->FindLocalVar(value->as<IRLocalRef>()->offset(), value->as<IRLocalRef>()->datatype()->moveSize());
            if (freg) {
                Reg creg = CastReg(*freg, local->datatype()->moveSize());
                TryCast(creg, *freg);
                this->writer->mov(
                    this->writer->ptr(
                        x86::Gp::rbp,
                        local->offset(),
                        local->datatype()->moveSize()
                    ),
                    creg
                );
                this->regalloc->SetVar(creg, local->offset(), RegisterAllocator::RegValue::Lifetime::UNTIL_ALLOC);
                break;
            }
        }
        case IRNode::NodeType::GLOBAL_REF: {
            Reg *freg = this->regalloc->FindLabel(value->as<IRGlobRef>()->getName(), value->as<IRGlobRef>()->getType()->moveSize());
            if (freg) {
                Reg creg = CastReg(*freg, local->datatype()->moveSize());
                TryCast(creg, *freg);
                this->writer->mov(
                    this->writer->ptr(
                        x86::Gp::rbp,
                        local->offset(),
                        local->datatype()->moveSize()
                    ),
                    creg
                );
                this->regalloc->SetVar(creg, local->offset(), RegisterAllocator::RegValue::Lifetime::UNTIL_ALLOC);
                break;
            }
        }
        default: {
            Reg src = this->EmitExpr(value, this->regalloc->Allocate(local->datatype()->moveSize(), true));
            this->TryCast(src, src);
            this->writer->mov(
                this->writer->ptr(
                    x86::Gp::rbp,
                    local->offset(),
                    local->datatype()->moveSize()
                ),
                src
            );
            this->regalloc->SetVar(src, local->offset(), RegisterAllocator::RegValue::Lifetime::UNTIL_ALLOC);
        }
    }
}

void WindEmitter::EmitIntoGlobalVar(IRGlobRef *global, IRNode *value) {
    switch (value->type()) {
        case IRNode::NodeType::LITERAL: {
            this->writer->mov(
                this->writer->ptr(
                    global->getName(),
                    0,
                    global->getType()->moveSize()
                ),
                value->as<IRLiteral>()->get()
            );
            break;
        }
        case IRNode::NodeType::LOCAL_REF: {
            Reg *freg = this->regalloc->FindLocalVar(value->as<IRLocalRef>()->offset(), value->as<IRLocalRef>()->datatype()->moveSize());
            if (freg) {
                Reg creg = CastReg(*freg, global->getType()->moveSize());
                TryCast(creg, *freg);
                this->writer->mov(
                    this->writer->ptr(
                        global->getName(),
                        0,
                        global->getType()->moveSize()
                    ),
                    creg
                );
                this->regalloc->SetLabel(creg, global->getName(), RegisterAllocator::RegValue::Lifetime::UNTIL_ALLOC);
                break;
            }
        }
        case IRNode::NodeType::GLOBAL_REF: {
            Reg *freg = this->regalloc->FindLabel(value->as<IRGlobRef>()->getName(), value->as<IRGlobRef>()->getType()->moveSize());
            if (freg) {
                Reg creg = CastReg(*freg, global->getType()->moveSize());
                TryCast(creg, *freg);
                this->writer->mov(
                    this->writer->ptr(
                        global->getName(),
                        0,
                        global->getType()->moveSize()
                    ),
                    creg
                );
                this->regalloc->SetLabel(creg, global->getName(), RegisterAllocator::RegValue::Lifetime::UNTIL_ALLOC);
                break;
            }
        }
        default: {
            Reg src = this->EmitExpr(value, this->regalloc->Allocate(global->getType()->moveSize(), true));
            this->TryCast(src, src);
            this->writer->mov(
                this->writer->ptr(
                    global->getName(),
                    0,
                    global->getType()->moveSize()
                ),
                src
            );
            this->regalloc->SetLabel(src, global->getName(), RegisterAllocator::RegValue::Lifetime::UNTIL_ALLOC);
        }
    }
}

void WindEmitter::ProcessGlobal(IRGlobalDecl *decl) {
    this->writer->BindSection(this->state->sections.data);
    this->writer->BindLabel(this->writer->NewLabel(decl->global()->getName()));
    if (decl->value()) {
        switch (decl->value()->type()) {
            case IRNode::NodeType::LITERAL: {
                switch (decl->global()->getType()->moveSize()) {
                    case 1: {
                        this->writer->Byte(decl->value()->as<IRLiteral>()->get());
                        break;
                    }
                    case 2: {
                        this->writer->Word(decl->value()->as<IRLiteral>()->get());
                        break;
                    }
                    case 4: {
                        this->writer->Dword(decl->value()->as<IRLiteral>()->get());
                        break;
                    }
                    case 8: {
                        this->writer->Qword(decl->value()->as<IRLiteral>()->get());
                        break;
                    }
                    default: {
                        throw std::runtime_error("Invalid global size");
                    }
                }
            }
        }
    } else {
        this->writer->Reserve(decl->global()->getType()->memSize());
    }
}