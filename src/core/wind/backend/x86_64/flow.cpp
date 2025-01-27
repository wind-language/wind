#include <wind/generation/IR.h>
#include <wind/backend/writer/writer.h>
#include <wind/backend/x86_64/backend.h>
#include <wind/bridge/flags.h>
#include <stdexcept>
#include <cstring>

void WindEmitter::EmitReturn(IRRet *ret) {
    IRNode *val = ret->get();
    if (val) {
        this->EmitExpr(
            val,
            CastReg(x86::Gp::rax, this->state->fn.ref->return_type->moveSize()),
            true
        );
    }
    this->EmitFnEpilogue();
}

Reg WindEmitter::EmitPtrGuard(IRPtrGuard *guard, Reg dst) {
    dst = this->EmitExpr(guard->getValue(), dst);
    this->writer->cmp(dst, 0);
    this->writer->je(
        this->state->RequestHandler(HandlerType::GUARD_HANDLER)
    );
    return dst;
}

Reg WindEmitter::EmitFnRef(IRFnRef *fn, Reg dst) {
    dst = CastReg(dst, 8);
    this->writer->lea(
        dst,
        this->writer->ptr(
            fn->name(),
            0,
            8
        )
    );
    return dst;
}


void WindEmitter::EmitTryCatch(IRTryCatch *try_catch) {
    std::map<HandlerType, WindEmitter::BackendState::HandlerDesc> old_handlers = this->state->handlers->active_handlers;
    uint16_t c_label = this->state->ActiveLabel();

    std::vector<uint16_t> handler_labels;
    for (auto &handler : try_catch->getHandlerMap()) {
        handler_labels.push_back(this->state->NewHandlerFlow(handler.first));
        this->state->SetHandler(
            handler.first,
            this->writer->LabelById(handler_labels.back())
        );
    }
    uint16_t end_label = this->state->NewLogicalFlow();
    uint16_t r_end_label = end_label;
    if (try_catch->getFinallyBody()) {
        r_end_label = this->state->NewLogicalFlow();
        this->writer->BindLabel(end_label);
        for (auto &stmt : try_catch->getFinallyBody()->get()) {
            this->EmitFnStatement(stmt.get());
        }
        this->writer->jmp(this->writer->LabelById(r_end_label));
    }

    this->writer->BindLabel(c_label);
    for (auto &stmt : try_catch->getTryBody()->get()) {
        this->ProcessStatement(stmt.get());
    }
    this->writer->jmp(this->writer->LabelById(r_end_label));
    
    int i=0;
    for (auto &handler : try_catch->getHandlerMap()) {
        this->writer->BindLabel(handler_labels[i]);
        for (auto &stmt : handler.second->get()) {
            this->ProcessStatement(stmt.get());
        }
        if (i != handler_labels.size()-1) {
            this->writer->jmp(this->writer->LabelById(end_label));
        }
        i++;
    }

    this->state->handlers->active_handlers = old_handlers;

    this->regalloc->Reset();
    this->writer->BindLabel(r_end_label);
}