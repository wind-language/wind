#include <wind/generation/IR.h>
#include <wind/backend/writer/writer.h>
#include <wind/backend/x86_64/backend.h>
#include <wind/bridge/flags.h>
#include <iostream>

void WindEmitter::EmitFnProCanary() {
    this->writer->rdtscp();
    this->writer->shl(x86::Gp::rdx, 32);
    this->writer->or_(x86::Gp::rdx, x86::Gp::rax);
    this->writer->mov(
        this->writer->ptr(
            x86::Seg::fs,
            0x40,
            8
        ),
        x86::Gp::rax
    );
    this->writer->mov(
        this->writer->ptr(
            x86::Gp::rbp,
            -8,
            8
        ),
        x86::Gp::rax
    );
    this->state->handlers->used_handlers.push_back({
        .type = WindEmitter::BackendState::HandlerDesc::Type::SYSTEM,
        .label = ".L__"+this->state->fn.ref->name()+".canary_fail",
        .handler_fn = SYSTEM_HANDLERS.at(CANARY_HANDLER),
        .op_handle = CANARY_HANDLER
    });
    /*
        Generate a value with rdtscp (timestamp + processor id) [ safe from thread migration ]
        Load the canary value in fs:0x40 and on the stack [rbp-8]
    */
}

void WindEmitter::EmitFnPrologue(IRFunction *fn) {
    this->state->UpdateFn(fn);
    if ( fn->flags & PURE_LOGUE || (fn->stack_size == 0) ) {}
    else { // if needed, create a stack frame
        
        this->writer->push(x86::Gp::rbp);
        this->writer->mov(x86::Gp::rbp, x86::Gp::rsp);
        uint16_t stack_size = NearPow2(fn->stack_size);
        if (stack_size>0) this->writer->sub(x86::Gp::rsp, align16(stack_size));

    }
    if (fn->isCanaryNeeded()) this->EmitFnProCanary();
}

void WindEmitter::EmitFnEpiCanary() {
    this->writer->mov(
        x86::Gp::rdx,
        this->writer->ptr(
            x86::Gp::rbp,
            -8,
            0
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
    this->writer->jne(".L__"+this->state->fn.ref->name()+".canary_fail");
}

void WindEmitter::EmitFnEpilogue() {
    if (this->state->fn.ref->isCanaryNeeded()) this->EmitFnEpiCanary();
    if ( this->state->fn.ref->flags & PURE_LOGUE || (this->state->fn.ref->stack_size == 0) ) {}
    else {
        this->writer->leave();
    }
    this->writer->ret();
}

void WindEmitter::EmitFnStatement(IRNode *node) {
    if (node->type() == IRNode::NodeType::ARG_DECL) {
        IRArgDecl *arg = node->as<IRArgDecl>();
        if (this->state->fn.in_arg_i<6) {
            this->regalloc->SetVar(SYSVABI_CNV[this->state->fn.in_arg_i], arg->local()->offset(), RegisterAllocator::RegValue::Lifetime::UNTIL_ALLOC);
            if (!this->state->fn.ref->ignore_stack_abi) {
                this->writer->mov(
                    this->writer->ptr(
                        x86::Gp::rbp,
                        arg->local()->offset(),
                        arg->local()->datatype()->moveSize()
                    ),
                    CastReg(SYSVABI_CNV[this->state->fn.in_arg_i], arg->local()->datatype()->moveSize())
                );
            }
        }
        this->state->fn.in_arg_i++;
    }
    else this->ProcessStatement(node);
}

void WindEmitter::ProcessFunction(IRFunction *fn) {
    this->writer->BindSection(this->state->sections.text);
    
    if (fn->flags & FN_EXTERN) { this->writer->Extern(fn->name()); return; }
    if (!fn->isDefined) return;
    if (fn->flags & FN_PUBLIC) { this->writer->Global(fn->name()); }


    this->writer->BindLabel(this->writer->NewLabel(fn->name()));
    this->writer->Comment(fn->metadata);
    this->EmitFnPrologue(fn);

    this->writer->OverflowChecks = !(fn->flags & PURE_ARITHM);
    this->regalloc->Reset(); // reset register allocator


    IRNode *pnode=nullptr;
    for (auto &stmt : fn->body()->get()) {
        pnode = stmt.get();
        this->EmitFnStatement(pnode);
    }

    if (!pnode || pnode->type() != IRNode::NodeType::RET) {
        this->EmitFnEpilogue();
    }

    // Handlers labels
    for (auto &handler : this->state->handlers->used_handlers) {
        this->writer->BindLabel(this->writer->NewLabel(handler.label));
        this->writer->lea(
            x86::Gp::rdi,
            this->writer->ptr(
                this->state->string(this->state->fn.ref->metadata),
                0,
                8
            )
        );
        this->writer->call(handler.handler_fn);
    }
}

void WindEmitter::EmitFnCallStackArg(IRNode *argv) {
    switch (argv->type()) {
        case IRNode::NodeType::LITERAL: {
            this->writer->push(argv->as<IRLiteral>()->get());
            break;
        }

        case IRNode::NodeType::LOCAL_REF: {
            IRLocalRef *local = argv->as<IRLocalRef>();
            Reg *reg = this->regalloc->FindLocalVar(local->offset(), local->datatype()->moveSize());
            if (reg) {
                TryCast(*reg, CastReg(*reg, 8));
                this->writer->push(CastReg(*reg, 8));
            } else {
                this->writer->push(
                    this->writer->ptr(
                        x86::Gp::rbp,
                        local->offset(),
                        local->datatype()->moveSize()
                    )
                );
            }
            break;
        }

        case IRNode::NodeType::GLOBAL_REF: {
            IRGlobRef *glob = argv->as<IRGlobRef>();
            this->writer->push(
                this->writer->ptr(
                    glob->getName(),
                    0,
                    glob->getType()->moveSize()
                )
            );
            break;
        }

        default: {
            Reg arg = this->EmitExpr(argv, this->regalloc->Allocate(8, true));
            this->writer->push(arg);
            this->regalloc->Free(arg);
        }
    }
}

void WindEmitter::EmitFnCallArgs(IRFnCall *call, std::vector<DataType*> &arg_types) {
    IRFunction *fn_ref = call->getRef();
    int arg_i=call->args().size()-1;
    for (;arg_i>=0;arg_i--) {
        if (arg_i>=fn_ref->ArgNum() && !fn_ref->isVariadic() && !fn_ref->isArgPush()) {
            throw std::runtime_error("Too many arguments");
        }

        arg_types.push_back(call->args()[arg_i]->inferType());
        IRNode *argv = call->args()[arg_i].get();
        if (arg_i<6 && !fn_ref->isArgPush()) {
            Reg arg_reg = this->EmitExpr(argv, SYSVABI_CNV[arg_i]);
            if (arg_i < fn_ref->ArgNum()) {
                TryCast(
                    CastReg(arg_reg, fn_ref->GetArgType(arg_i)->moveSize()),
                    arg_reg
                );
            }
            this->regalloc->SetLifetime(SYSVABI_CNV[arg_i], RegisterAllocator::RegValue::Lifetime::FN_CALL);
        }
        else {
            this->EmitFnCallStackArg(argv);
        }
    }
}

Reg WindEmitter::EmitFnCall(IRFnCall *fn_call, Reg dst) {
    IRFunction *fn_ref = fn_call->getRef();
    
    std::vector<DataType*> arg_types;
    this->EmitFnCallArgs(fn_call, arg_types);

    uint16_t argstack_size = (fn_call->args().size() > 6 ? (fn_call->args().size()-6)*8 : 0);
    if (fn_ref->isVariadic()) {
        this->writer->xor_(x86::Gp::rax, x86::Gp::rax);
    }
    else if (fn_ref->isArgPush()) {
        this->writer->mov(x86::Gp::rax, fn_call->args().size());
        this->writer->mov(
            x86::Gp::rbx,
            get64bitDesc(arg_types)
        );
        argstack_size = fn_call->args().size()*8;
    }

    this->regalloc->Reset();
    this->writer->call(fn_ref->name());

    if (argstack_size>0) this->writer->add(x86::Gp::rsp, argstack_size);
    if (dst.id != 0 && dst.size != 0) {
        this->writer->mov(dst, x86::Gp::rax);
    }
    dst.size = fn_ref->return_type->moveSize();
    return dst;
}