#include <wind/generation/IR.h>
#include <wind/backend/writer/writer.h>
#include <wind/backend/x86_64/backend.h>
#include <wind/bridge/opt_flags.h>

#include <iostream>

unsigned NearPow2(unsigned n) {
    if (n==0) return 1;
    n--;
    n |= n >> 1; n |= n >> 2; n |= n >> 4; n |= n >> 8; n |= n >> 16;
    return n+1;
}

unsigned align16(unsigned n) {
    // System V ABI is annoying and requires 16-byte stack alignment
    int mask = ~(0xF);
    int aligned = n & mask;
    if (n != aligned) {
        aligned += 16;
    }
    return aligned;
}

void WindEmitter::EmitFnPrologue(IRFunction *fn) {
    this->current_fn = fn;
    if ( fn->flags & PURE_LOGUE || (!fn->stack_size && !fn->isCallSub()) ) {}
    else {
        this->writer->push(x86::Gp::rbp);
        this->writer->mov(x86::Gp::rbp, x86::Gp::rsp);
        unsigned stack_size = NearPow2(fn->stack_size + (fn->isCallSub() ? 8 : 0));
        this->writer->sub(x86::Gp::rsp,
            align16(stack_size)
        );
    }
}

void WindEmitter::EmitFnEpilogue() {
    if ( this->current_fn->flags & PURE_LOGUE || (!this->current_fn->stack_size && !this->current_fn->isCallSub()) ) {}
    else {
        this->writer->leave();
    }
    this->writer->ret();
    this->current_fn = nullptr;
}

void WindEmitter::ProcessFunction(IRFunction *func) {
    this->writer->BindSection(this->textSection);
    if (func->flags & FN_EXTERN) {
        this->writer->Extern(func->name());
        return;
    }
    else if (func->name() == "main") {
        this->writer->Global("main");
    }
    this->writer->BindLabel(this->writer->NewLabel(func->name()));
    this->EmitFnPrologue(func);

    IRNode *last = nullptr;
    for (auto &stmt : func->body()->get()) {
        last = stmt.get();
        this->ProcessStatement(last);
    }

    if (!last || last->type() != IRNode::NodeType::RET) {
        this->EmitFnEpilogue();
    }
}