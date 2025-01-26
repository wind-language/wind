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