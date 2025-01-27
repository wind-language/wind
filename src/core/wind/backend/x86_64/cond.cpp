#include <wind/generation/IR.h>
#include <wind/backend/writer/writer.h>
#include <wind/backend/x86_64/backend.h>
#include <wind/bridge/flags.h>
#include <stdexcept>
#include <iostream>


void WindEmitter::EmitCondJump(IRNode *cond, uint16_t label, bool invert) {
    if (cond->type() != IRNode::NodeType::BIN_OP) {
        Reg val = this->EmitExpr(cond, this->regalloc->Allocate(8, false), false);
        this->writer->cmp(val, 0);
        this->state->jmp_map[IRBinOp::Operation::NOTEQ][cond->inferType()->isSigned()][invert](label);
        return;
    }
    Reg val = this->EmitExpr(cond, this->regalloc->Allocate(8, false), false, true);
    auto jmp_it = this->state->jmp_map.find(cond->as<IRBinOp>()->operation());
    if (jmp_it == this->state->jmp_map.end()) {
        this->writer->cmp(val, 0);
        this->state->jmp_map[IRBinOp::Operation::NOTEQ][cond->inferType()->isSigned()][invert](label);
        return;
    }
    jmp_it->second[cond->inferType()->isSigned()][invert](label);
}

void WindEmitter::EmitBranch(IRBranching *branch) {
    std::vector<uint16_t> labels;
    int Nb = branch->getBranches().size();
    for (int i=0;i<Nb;i++) {
        labels.push_back(this->state->NewLogicalFlow());        
    }
    uint16_t end_label = this->state->NewLogicalFlow();
    for (int i=0;i<Nb;i++) {
        IRNode *c = branch->getBranches()[i].condition.get();
        EmitCondJump(c, labels[i]);
    }
    // emit else branch
    if (branch->getElseBranch() != nullptr) {
        for (auto &stmt : branch->getElseBranch()->get()) {
            this->ProcessStatement(stmt.get());
        }
        this->regalloc->Reset();
    }
    this->writer->jmp(this->writer->LabelById(end_label));
    for (int i=0;i<Nb;i++) {
        this->writer->BindLabel(labels[i]);
        for (auto &stmt : branch->getBranches()[i].body.get()->as<IRBody>()->get()) {
            this->ProcessStatement(stmt.get());
        }
        if (i != Nb-1) {
            this->writer->jmp(this->writer->LabelById(end_label));
        }
        this->regalloc->Reset();
    }
    this->writer->BindLabel(end_label);
}

void WindEmitter::EmitLoop(IRLooping *loop) {
    uint16_t loop_label = this->state->NewLogicalFlow();
    uint16_t end_label = this->state->NewLogicalFlow();
    WindEmitter::BackendState::LogicalFlow *backup = this->state->l_flow;
    this->state->l_flow = new WindEmitter::BackendState::LogicalFlow();
    this->state->l_flow->start = this->writer->LabelById(loop_label);
    this->state->l_flow->end = this->writer->LabelById(end_label);

    this->writer->BindLabel(loop_label);
    this->regalloc->Reset();
    this->EmitCondJump(loop->getCondition(), end_label, true);
    for (auto &stmt : loop->getBody()->get()) {
        this->ProcessStatement(stmt.get());
    }
    this->writer->jmp(this->writer->LabelById(loop_label));
    this->writer->BindLabel(end_label);
    this->regalloc->Reset();
    delete this->state->l_flow;
    this->state->l_flow = backup;
}


void WindEmitter::EmitContinue() {
    if (this->state->l_flow == nullptr) {
        throw std::runtime_error("Continue statement outside of loop");
    }
    this->writer->jmp(this->state->l_flow->start);
}

void WindEmitter::EmitBreak() {
    if (this->state->l_flow == nullptr) {
        throw std::runtime_error("Break statement outside of loop");
    }
    this->writer->jmp(this->state->l_flow->end);
}