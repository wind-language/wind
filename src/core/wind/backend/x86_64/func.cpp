#include <wind/generation/IR.h>
#include <wind/backend/writer/writer.h>
#include <wind/backend/x86_64/backend.h>
#include <wind/bridge/opt_flags.h>
#include <wind/backend/x86_64/expr_macros.h>

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
    uint16_t metadata_ros_i=0;
    if (!(fn->flags & PURE_STCHK)) {
        this->rostrs.push_back(
            fn->name() + "(...) [" + fn->metadata + "]"
        );
        metadata_ros_i = this->rostr_i++;
    }
    this->current_fn = new FunctionDesc({fn, metadata_ros_i});
    if ( fn->flags & PURE_LOGUE || (!fn->stack_size && !fn->isCallSub()) ) {}
    else {
        this->writer->push(x86::Gp::rbp);
        this->writer->mov(x86::Gp::rbp, x86::Gp::rsp);
        unsigned stack_size = NearPow2(fn->stack_size + (fn->isCallSub() ? 16 : 0));
        this->writer->sub(x86::Gp::rsp,
            align16(stack_size)
        );
    }
    if (!(fn->flags & PURE_STCHK) && this->current_fn->fn->canary_needed) {
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
    if (!(this->current_fn->fn->flags & PURE_STCHK) && this->current_fn->fn->canary_needed) {
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
            "." + this->current_fn->fn->name() + "_stackfail"
        );
    }
    if ( this->current_fn->fn->flags & PURE_LOGUE || (!this->current_fn->fn->stack_size && !this->current_fn->fn->isCallSub()) ) {}
    else {
        this->writer->leave();
    }
    this->writer->ret();
}

const Reg SYSVABI_CNV[6] = {
    x86::Gp::rdi, x86::Gp::rsi, x86::Gp::rdx, x86::Gp::rcx, 
    x86::Gp::r8, x86::Gp::r9
};

bool isFnDef(std::vector<std::string> def_fns, std::string name) {
    for (auto fn : def_fns) {
        if (fn == name) return true;
    }
    return false;
}

void WindEmitter::ProcessFunction(IRFunction *func) {
    this->writer->BindSection(this->textSection);
    
    if ((func->flags & FN_EXTERN || !func->isDefined)) {
        if (!isFnDef(this->program->getDefFns(), func->name())) {
            // either extern or wrongly externed
            this->writer->Extern(func->name());
        }
        return;
    }
    else if (func->flags & FN_PUBLIC || func->name() == "main") {
        this->writer->Global(func->name());
    }
    uint8_t fn_label =this->writer->NewLabel(func->name());
    this->writer->BindLabel(fn_label);
    
    this->EmitFnPrologue(func);

    this->regalloc.FreeAllRegs();
    IRNode *last = nullptr;
    int arg_i=0;
    for (auto &stmt : func->body()->get()) {
        last = stmt.get();

        if (last->type() == IRNode::NodeType::ARG_DECL) {
            IRArgDecl *arg = last->as<IRArgDecl>();
            if (arg_i<6) {
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
        this->writer->jmp(
            "__WD_canary_fail"
        );
    }
    if (!(func->flags & PURE_STCHK)) {
        std::string h_lab = "";
        for (auto handle : this->current_fn->handlers) {
            if (!handle.second.first) continue;
            h_lab = HANDLER_LABEL(func->name(), handle.first);
            this->writer->BindLabel(this->writer->NewLabel(h_lab));
            this->writer->lea(x86::Gp::rdi, this->writer->ptr(
                ".ros"+std::to_string(this->current_fn->metadata_l),
                0,
                8
            ));
            this->writer->jmp(handle.second.second);
        }
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
        if (arg_i<6) {
            this->EmitExpr(call->args()[arg_i].get(), SYSVABI_CNV[arg_i]);
            this->regalloc.SetVar(SYSVABI_CNV[arg_i], 0, RegisterAllocator::RegValue::Lifetime::FN_CALL);
        } else {
            IRNode *argv = call->args()[arg_i].get();
            if (argv->is<IRLiteral>()) {
                this->writer->push(argv->as<IRLiteral>()->get());
            } else if (argv->is<IRGlobRef>()) {
                this->writer->push(this->writer->ptr(
                    argv->as<IRGlobRef>()->getName(),
                    0,
                    argv->as<IRGlobRef>()->getType()->moveSize()
                ));
            } else if (argv->is<IRStringLiteral>()) {
                this->rostrs.push_back(argv->as<IRStringLiteral>()->get());
                this->writer->push(this->writer->ptr(
                    ".ros"+std::to_string(this->rostr_i++),
                    0,
                    8
                ));
            }
            else if (argv->is<IRFnRef>()) {
                this->writer->push(
                    this->writer->ptr(
                        argv->as<IRFnRef>()->name(),
                        0,
                        8
                    )
                );
            } else {
                Reg arg = this->regalloc.Allocate(8, false);
                this->EmitExpr(argv, arg);
                this->writer->push(arg);
                this->regalloc.Free(arg);
            }
        }
    }

    if (call->getRef()->flags & FN_VARIADIC) {
        this->writer->xor_(x86::Gp::rax, x86::Gp::rax);
    }
    this->regalloc.FreeAllRegs();
    this->writer->call(call->name());
    uint16_t stack_args_size = (call->args().size() > 6 ? (call->args().size()-6)*8 : 0);
    if (stack_args_size!=0) {
        this->writer->add(x86::Gp::rsp, stack_args_size);
    }
    if (dst.id != 0) {
        this->writer->mov(dst, this->CastReg(x86::Gp::rax, dst.size));
    }
    return dst;
}