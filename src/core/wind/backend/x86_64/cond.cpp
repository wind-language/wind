#include <wind/generation/IR.h>
#include <wind/backend/writer/writer.h>
#include <wind/backend/x86_64/backend.h>
#include <wind/bridge/opt_flags.h>
#include <stdexcept>
#include <iostream>

void WindEmitter::EmitCJump(IRNode *node, uint16_t label, bool invert) {
    Reg rinfo = this->EmitExpr(node, x86::Gp::rax, true);
    if (node->type() != IRNode::NodeType::BIN_OP) {
        this->writer->test(x86::Gp::rax, x86::Gp::rax);
    } else {
        IRBinOp *binop = node->as<IRBinOp>();
        auto jmp_it = this->jmp_map.find(binop->operation());
        if (jmp_it == this->jmp_map.end()) {
            this->writer->test(x86::Gp::rax, x86::Gp::rax);
        } else {
            jmp_it->second[rinfo.signed_value][invert ? 1 : 0](label);
            return;
        }
    }
    this->writer->je(this->writer->LabelById(label));
}

void WindEmitter::EmitLoop(IRLooping *loop) {
    uint8_t start = this->writer->NewLabel(".L"+std::to_string(this->ljl_i++));
    uint8_t end = this->writer->NewLabel(".L"+std::to_string(this->ljl_i++));
    this->writer->BindLabel(start);
    this->regalloc.FreeAllRegs();
    this->EmitCJump(loop->getCondition(), end, true);
    FlowDesc *old = this->c_flow_desc;
    this->c_flow_desc = new FlowDesc({start, end});
    for (auto &node : loop->getBody()->get()) {
        this->ProcessStatement(node.get());
    }
    this->writer->jmp(this->writer->LabelById(start));
    this->writer->BindLabel(end);
    this->c_flow_desc = old;
}

void WindEmitter::EmitBranch(IRBranching *branch) {
    int N = branch->getBranches().size();
    uint8_t *labels = new uint8_t[N];
    for (int i = 0; i < N; i++) {
        labels[i] = this->writer->NewLabel(".L"+std::to_string(this->ljl_i++));
    }
    uint8_t end = this->writer->NewLabel(".L"+std::to_string(this->ljl_i++));
    for (int i = 0; i < N; i++) {
        this->EmitCJump(branch->getBranches()[i].condition.get(), labels[i], false);
    }
    if (branch->getElseBranch() != nullptr) {
        for (auto &statement : branch->getElseBranch()->get()) {
            this->ProcessStatement(statement.get());
        }
    }
    this->writer->jmp(this->writer->LabelById(end));
    for (int i = 0; i < N; i++) {
        this->writer->BindLabel(labels[i]);
        IRBody *body = branch->getBranches()[i].body.get()->as<IRBody>();
        for (auto &statement : body->get()) {
            this->ProcessStatement(statement.get());
        }
        if (i < N-1)
            this->writer->jmp(this->writer->LabelById(end));
    }
    this->writer->BindLabel(end);
}