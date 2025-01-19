#include <wind/generation/IR.h>
#include <wind/backend/writer/writer.h>
#include <wind/backend/x86_64/backend.h>
#include <wind/bridge/flags.h>
#include <stdexcept>
#include <cstring>

void WindEmitter::EmitReturn(IRRet *ret) {
    IRNode *val = ret->get();
    if (val) {
        Reg retv = this->EmitExpr(
            val,
            this->CastReg(
                x86::Gp::rax,
                this->current_fn->fn->return_type->moveSize()
            )
        );
    }
    this->EmitFnEpilogue();
}

void WindEmitter::EmitTryCatch(IRTryCatch *trycatch) {
    std::map<std::string, UserHandlerDesc> old_handlers = this->current_fn->active_handlers;
    for (auto &handling : trycatch->getHandlerMap()) {
        std::vector<std::string> instructions = HANDLER_INSTR_MAP.find(handling.first)->second;
        std::string handler_str = (".L_usr_"+std::to_string(this->ljl_i)+"_"+instructions[0]+".handler").c_str();
        const char *handler_label = (const char*)malloc(handler_str.size());
        memcpy((void*)handler_label, handler_str.c_str(), handler_str.size()); // We all love unsafe code, C++ GC is really frustrating
        this->current_fn->user_handlers.push_back({
            handler_label,
            this->ljl_i,
            handling.second,
            old_handlers
        });
        for (std::string instruction : instructions) {
            this->current_fn->active_handlers[instruction] = {
                handler_label,
                this->ljl_i,
                handling.second,
                old_handlers
            };
        }
    }
    for (auto &statement : trycatch->getTryBody()->get()) {
        this->ProcessStatement(statement.get());
    }
    this->current_fn->active_handlers = old_handlers;
    this->writer->BindLabel(this->writer->NewLabel(".L"+std::to_string(this->ljl_i++))); // after try
}