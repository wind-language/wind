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

const Reg SYSVABI_CNV[8] = {
    x86::Gp::rdi, x86::Gp::rsi, x86::Gp::rdx, x86::Gp::rcx, 
    x86::Gp::r8, x86::Gp::r9, x86::Gp::r10, x86::Gp::r11
};

void WindEmitter::ProcessFunction(IRFunction *func) {
    this->writer->BindSection(this->textSection);
    if (func->flags & FN_EXTERN) {
        this->writer->Extern(func->name());
        return;
    }
    else if (func->flags & FN_PUBLIC || func->name() == "main") {
        this->writer->Global(func->name());
    }
    this->writer->BindLabel(this->writer->NewLabel(func->name()));
    this->EmitFnPrologue(func);


    this->regalloc.PostCall(); // trick to free all registers
    IRNode *last = nullptr;
    int arg_i=0;
    for (auto &stmt : func->body()->get()) {
        last = stmt.get();

        if (last->type() == IRNode::NodeType::ARG_DECL) {
            IRArgDecl *arg = last->as<IRArgDecl>();
            if (arg_i<=8) {
                this->regalloc.SetVar(SYSVABI_CNV[arg_i], arg->local()->offset(), RegisterAllocator::RegValue::Lifetime::UNTIL_ALLOC);
                if (!func->ignore_stack_abi) {
                    this->writer->mov(
                        this->writer->ptr(
                            x86::Gp::rbp,
                            arg->local()->offset(),
                            arg->local()->datatype()->moveSize()
                        ),
                        CastReg(SYSVABI_CNV[arg_i], arg->local()->datatype()->moveSize())
                    );
                }
            } else {
                this->writer->pop(x86::Gp::rax);
                this->writer->mov(
                    this->writer->ptr(
                        x86::Gp::rbp,
                        arg->local()->offset(),
                        arg->local()->datatype()->moveSize()
                    ),
                    CastReg(x86::Gp::rax, arg->local()->datatype()->moveSize())
                );
            }
            arg_i++;
        }

        else { this->ProcessStatement(last); }
    }

    if (!last || last->type() != IRNode::NodeType::RET) {
        this->EmitFnEpilogue();
    }
}

void WindEmitter::EmitFnCall(IRFnCall *call, Reg dst) {

    int arg_i = call->args().size()-1;
    for (;arg_i>=0;arg_i--) {
        if (arg_i>=call->getRef()->ArgNum() && !(call->getRef()->flags & FN_VARIADIC)) {
            throw std::runtime_error("Too many arguments");
        }
        if (arg_i<=8) {
            this->EmitExpr(call->args()[arg_i].get(), SYSVABI_CNV[arg_i]);
            this->regalloc.SetVar(SYSVABI_CNV[arg_i], 0, RegisterAllocator::RegValue::Lifetime::FN_CALL);
        } else {
            Reg arg = this->regalloc.Allocate(8, false);
            this->EmitExpr(call->args()[arg_i].get(), arg);
            this->writer->push(arg);
            this->regalloc.Free(arg);
        }
    }

    if (call->getRef()->flags & FN_VARIADIC) {
        this->writer->xor_(x86::Gp::rax, x86::Gp::rax);
    }
    this->regalloc.PostCall();
    this->writer->call(call->name());
    if (dst.id != 0) {
        this->writer->mov(dst, this->CastReg(x86::Gp::rax, dst.size));
    }
}