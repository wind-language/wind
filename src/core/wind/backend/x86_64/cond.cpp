#include <wind/generation/IR.h>
#include <wind/backend/writer/writer.h>
#include <wind/backend/x86_64/backend.h>
#include <wind/bridge/opt_flags.h>
#include <stdexcept>
#include <iostream>

void WindEmitter::EmitCJump(IRNode *node, uint8_t label, bool invert) {
    this->EmitExpr(node, x86::Gp::rax, true);
    if (node->type() != IRNode::NodeType::BIN_OP) {
        this->writer->test(x86::Gp::rax, x86::Gp::rax);
    } else {
        IRBinOp *binop = node->as<IRBinOp>();
        auto jmp_it = this->jmp_map.find(binop->operation());
        if (jmp_it == this->jmp_map.end()) {
            this->writer->test(x86::Gp::rax, x86::Gp::rax);
        } else {
            jmp_it->second[invert ? 1 : 0](label);
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
    for (auto &node : loop->getBody()->get()) {
        this->ProcessStatement(node.get());
    }
    this->writer->jmp(this->writer->LabelById(start));
    this->writer->BindLabel(end);
}