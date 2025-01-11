#include <wind/generation/IR.h>
#include <wind/backend/writer/writer.h>
#include <wind/backend/x86_64/backend.h>
#include <wind/bridge/opt_flags.h>
#include <stdexcept>

void WindEmitter::EmitReturn(IRRet *ret) {
    IRNode *val = ret->get();
    if (val) {
        Reg retv = this->EmitExpr(
            val,
            this->CastReg(
                x86::Gp::rax,
                this->current_fn->return_type->moveSize()
            )
        );
    }
    this->EmitFnEpilogue();
}