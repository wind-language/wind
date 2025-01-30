#include <wind/generation/IR.h>
#include <wind/backend/writer/writer.h>
#include <wind/backend/x86_64/backend.h>
#include <wind/bridge/flags.h>
#include <iostream>

void WindEmitter::EmitStructIntoLocal(IRLocalRef *ref, IRStructValue *struct_val) {
    for (auto &field : struct_val->getFields()) {
        if (field.second->is<IRStructValue>()) {
            this->EmitStructIntoLocal(
                new IRLocalRef(
                    ref->offset() + field.first.type->memSize(),
                    field.first.type
                ),
                field.second->as<IRStructValue>()
            );
        }
        else {
            this->EmitIntoLocalVar(
                ref,
                field.second,
                field.first.offset,
                field.first.type
            );
        }
    }
}

Reg WindEmitter::EmitLocFieldAcc(IRLocFieldAccess *acc, Reg dst) {
    return this->EmitLocalVar(acc->getLocal(), dst, acc->getOffset(), acc->inferType());
}