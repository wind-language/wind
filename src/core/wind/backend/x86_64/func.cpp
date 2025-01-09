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
        unsigned stack_size = NearPow2(fn->stack_size + (fn->isCallSub() ? 16 : 0));
        this->writer->sub(x86::Gp::rsp,
            align16(stack_size)
        );
    }
    if (!(fn->flags & PURE_STCHK) && this->current_fn->canary_needed) {
        this->writer->rdtscp();
        this->writer->shl(x86::Gp::rdx, 32);
        this->writer->or_(x86::Gp::rdx, x86::Gp::rax);
        this->writer->mov(
            x86::Gp::rax,
            this->writer->ptr(
                x86::Seg::fs,
                0x40,
                8
            )
        );
        this->writer->mov(
            this->writer->ptr(
                x86::Gp::rbp,
                -8,
                8
            ),
            x86::Gp::rax
        );
    }
}

void WindEmitter::EmitFnEpilogue() {
    if (!(this->current_fn->flags & PURE_STCHK) && this->current_fn->canary_needed) {
        this->writer->mov(
            x86::Gp::rdx,
            this->writer->ptr(
                x86::Gp::rbp,
                -8,
                8
            )
        );
        this->writer->sub(
            x86::Gp::rdx,
            this->writer->ptr(
                x86::Seg::fs,
                0x40,
                8
            )
        );
        this->writer->jne(
            "." + this->current_fn->name() + "_stackfail"
        );
    }
    if ( this->current_fn->flags & PURE_LOGUE || (!this->current_fn->stack_size && !this->current_fn->isCallSub()) ) {}
    else {
        this->writer->leave();
    }
    this->writer->ret();
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
    uint8_t fn_label =this->writer->NewLabel(func->name());
    this->writer->BindLabel(fn_label);
    
    uint8_t metadata_ros_i=0;
    if (!(func->flags & PURE_STCHK)) {
        this->rostrs.push_back(
            func->name() + "(...) [" + func->metadata + "]"
        );
        metadata_ros_i = this->rostr_i++;
        this->writer->lea( // Load function info in r15 [ : HANDLER : ]
            x86::Gp::r15,
            this->writer->ptr(
                ".ros"+std::to_string(metadata_ros_i),
                0,
                8
            )
        );
    }
    
    this->EmitFnPrologue(func);

    this->regalloc.FreeAllRegs();
    IRNode *last = nullptr;
    int arg_i=0;
    for (auto &stmt : func->body()->get()) {
        last = stmt.get();

        if (last->type() == IRNode::NodeType::ARG_DECL) {
            IRArgDecl *arg = last->as<IRArgDecl>();
            if (arg_i<8) {
                this->regalloc.SetVar(SYSVABI_CNV[arg_i], arg->local()->offset(), RegisterAllocator::RegValue::Lifetime::UNTIL_ALLOC);
                if (!func->ignore_stack_abi) {
                    this->writer->mov(
                        this->writer->ptr(
                            x86::Gp::rbp,
                            -arg->local()->offset(),
                            arg->local()->datatype()->moveSize()
                        ),
                        CastReg(SYSVABI_CNV[arg_i], arg->local()->datatype()->moveSize())
                    );
                }
            } else {
                /* this->writer->pop(x86::Gp::rax);
                this->writer->mov(
                    this->writer->ptr(
                        x86::Gp::rbp,
                        arg->local()->offset(),
                        arg->local()->datatype()->moveSize()
                    ),
                    CastReg(x86::Gp::rax, arg->local()->datatype()->moveSize())
                ); */
                throw std::runtime_error(">8 fn args not supported");
            }
            arg_i++;
        }

        else { this->ProcessStatement(last); }
    }

    if (!last || last->type() != IRNode::NodeType::RET) {
        this->EmitFnEpilogue();
    }
    if (!(func->flags & PURE_STCHK) && func->canary_needed) {
        this->writer->BindLabel(
            this->writer->NewLabel("." + func->name() + "_stackfail")
        );
        this->writer->lea( // Load function info in r15 [ : HANDLER : ]
            x86::Gp::r15,
            this->writer->ptr(
                ".ros"+std::to_string(metadata_ros_i),
                0,
                8
            )
        );
        this->writer->jmp(
            "__WD_canary_fail"
        );
    }
    this->current_fn = nullptr;
}

Reg WindEmitter::EmitFnCall(IRFnCall *call, Reg dst) {
    int arg_i = call->args().size()-1;
    dst.size = call->getRef()->return_type->moveSize();
    for (;arg_i>=0;arg_i--) {
        if (arg_i>=call->getRef()->ArgNum() && !(call->getRef()->flags & FN_VARIADIC)) {
            throw std::runtime_error("Too many arguments");
        }
        if (arg_i<8) {
            this->EmitExpr(call->args()[arg_i].get(), SYSVABI_CNV[arg_i]);
            this->regalloc.SetVar(SYSVABI_CNV[arg_i], 0, RegisterAllocator::RegValue::Lifetime::FN_CALL);
        } else {
            /* Reg arg = this->regalloc.Allocate(8, false);
            this->EmitExpr(call->args()[arg_i].get(), arg);
            this->writer->push(arg);
            this->regalloc.Free(arg); */
            throw std::runtime_error(">8 fn args not supported");
        }
    }

    if (call->getRef()->flags & FN_VARIADIC) {
        this->writer->xor_(x86::Gp::rax, x86::Gp::rax);
    }
    this->regalloc.FreeAllRegs();
    if (!(this->current_fn->flags & PURE_STCHK)) {
        this->writer->mov(
            this->writer->ptr(
                x86::Gp::rbp,
                -16,
                8
            ),
            x86::Gp::r15
        );
    }
    this->writer->call(call->name());
    if (!(this->current_fn->flags & PURE_STCHK)) {
        this->writer->mov(
            x86::Gp::r15,
            this->writer->ptr(
                x86::Gp::rbp,
                -16,
                8
            )
        );
    }
    if (dst.id != 0) {
        this->writer->mov(dst, this->CastReg(x86::Gp::rax, dst.size));
    }
    return dst;
}