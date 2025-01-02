#include <wind/generation/IR.h>
#include <wind/backend/writer/writer.h>
#include <wind/backend/x86_64/backend.h>
#include <wind/bridge/opt_flags.h>
#include <iostream>

void WindEmitter::ProcessGlobalDecl(IRGlobalDecl *decl) {
    this->writer->BindSection(this->dataSection);
    this->writer->Global(decl->global()->getName());
    this->writer->BindLabel(this->writer->NewLabel(decl->global()->getName()));
    this->writer->Align(decl->global()->getType()->memSize());
    IRNode *value = decl->value();
    if (value) {
        if (value->is<IRStringLiteral>()) {
            this->writer->String(value->as<IRStringLiteral>()->get());
        } else if (value->is<IRLiteral>()) {
            switch (decl->global()->getType()->moveSize()) {
                case DataType::Sizes::BYTE:
                    this->writer->Byte(value->as<IRLiteral>()->get());
                    break;
                case DataType::Sizes::WORD:
                    this->writer->Word(value->as<IRLiteral>()->get());
                    break;
                case DataType::Sizes::DWORD:
                    this->writer->Dword(value->as<IRLiteral>()->get());
                    break;
                case DataType::Sizes::QWORD:
                    this->writer->Qword(value->as<IRLiteral>()->get());
                    break;
            }
        }
    } else {
        this->writer->Reserve(decl->global()->getType()->memSize());
    }
}

void WindEmitter::EmitLocDecl(IRVariableDecl *decl) {
    IRLocalRef *local = decl->local();
    IRNode *value = decl->value();
    if (value) this->EmitIntoLoc(local, value);
}

void WindEmitter::EmitIntoLoc(IRLocalRef *ref, IRNode *value) {
    Reg src = this->regalloc.Allocate(ref->datatype()->moveSize(), false);
    this->EmitExpr(value, src);
    this->writer->mov(
        this->writer->ptr(
            x86::Gp::rbp,
            -ref->offset(),
            ref->datatype()->moveSize()
        ),
        src
    );
    this->regalloc.SetVar(src, ref->offset(), RegisterAllocator::RegValue::Lifetime::UNTIL_ALLOC);
}

void WindEmitter::EmitGlobRef(IRGlobRef *ref, Reg dst) {
    this->writer->mov(
        this->CastReg(dst, ref->getType()->moveSize()),
        this->writer->ptr(
            ref->getName(),
            0,
            ref->getType()->moveSize()
        )
    );
    this->regalloc.SetVar(dst, 0, RegisterAllocator::RegValue::Lifetime::UNTIL_ALLOC);
}