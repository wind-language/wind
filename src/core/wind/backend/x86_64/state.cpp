#include <wind/backend/x86_64/backend.h>


std::string WindEmitter::BackendState::string(std::string str) {
    if (this->strings.strmap.find(str) == this->strings.strmap.end()) {
        this->strings.strmap[str] = ".rostr" + std::to_string(this->strings.str_i++);
    }
    return this->strings.strmap[str];
}

void WindEmitter::BackendState::EmitStrings() {
    emitter.writer->BindSection(this->sections.rodata);
    for (auto &str : this->strings.strmap) {
        emitter.writer->BindLabel(emitter.writer->NewLabel(str.second));
        emitter.writer->String(str.first);
    }
}

void WindEmitter::SetupJumpMap() {

    // a cmp between left and right is always done before calling one of these functions
    state->jmp_map[IRBinOp::Operation::EQ][0][0] = [this](uint16_t label) { this->writer->je(this->writer->LabelById(label)); };
    state->jmp_map[IRBinOp::Operation::EQ][0][1] = [this](uint16_t label) { this->writer->jne(this->writer->LabelById(label)); };
    state->jmp_map[IRBinOp::Operation::EQ][1][0] = [this](uint16_t label) { this->writer->je(this->writer->LabelById(label)); };
    state->jmp_map[IRBinOp::Operation::EQ][1][1] = [this](uint16_t label) { this->writer->jne(this->writer->LabelById(label)); };

    state->jmp_map[IRBinOp::Operation::LESS][0][0] = [this](uint16_t label) { this->writer->jb(this->writer->LabelById(label)); };
    state->jmp_map[IRBinOp::Operation::LESS][0][1] = [this](uint16_t label) { this->writer->jnb(this->writer->LabelById(label)); };
    state->jmp_map[IRBinOp::Operation::LESS][1][0] = [this](uint16_t label) { this->writer->jl(this->writer->LabelById(label)); };
    state->jmp_map[IRBinOp::Operation::LESS][1][1] = [this](uint16_t label) { this->writer->jge(this->writer->LabelById(label)); };

    state->jmp_map[IRBinOp::Operation::GREATER][0][0] = [this](uint16_t label) { this->writer->ja(this->writer->LabelById(label)); };
    state->jmp_map[IRBinOp::Operation::GREATER][0][1] = [this](uint16_t label) { this->writer->jbe(this->writer->LabelById(label)); };
    state->jmp_map[IRBinOp::Operation::GREATER][1][0] = [this](uint16_t label) { this->writer->jg(this->writer->LabelById(label)); };
    state->jmp_map[IRBinOp::Operation::GREATER][1][1] = [this](uint16_t label) { this->writer->jle(this->writer->LabelById(label)); };

    state->jmp_map[IRBinOp::Operation::LESSEQ][0][0] = [this](uint16_t label) { this->writer->jbe(this->writer->LabelById(label)); };
    state->jmp_map[IRBinOp::Operation::LESSEQ][0][1] = [this](uint16_t label) { this->writer->ja(this->writer->LabelById(label)); };
    state->jmp_map[IRBinOp::Operation::LESSEQ][1][0] = [this](uint16_t label) { this->writer->jle(this->writer->LabelById(label)); };
    state->jmp_map[IRBinOp::Operation::LESSEQ][1][1] = [this](uint16_t label) { this->writer->jg(this->writer->LabelById(label)); };

    state->jmp_map[IRBinOp::Operation::GREATEREQ][0][0] = [this](uint16_t label) { this->writer->jae(this->writer->LabelById(label)); };
    state->jmp_map[IRBinOp::Operation::GREATEREQ][0][1] = [this](uint16_t label) { this->writer->jbe(this->writer->LabelById(label)); };
    state->jmp_map[IRBinOp::Operation::GREATEREQ][1][0] = [this](uint16_t label) { this->writer->jge(this->writer->LabelById(label)); };
    state->jmp_map[IRBinOp::Operation::GREATEREQ][1][1] = [this](uint16_t label) { this->writer->jl(this->writer->LabelById(label)); };

    state->jmp_map[IRBinOp::Operation::NOTEQ][0][0] = [this](uint16_t label) { this->writer->jne(this->writer->LabelById(label)); };
    state->jmp_map[IRBinOp::Operation::NOTEQ][0][1] = [this](uint16_t label) { this->writer->je(this->writer->LabelById(label)); };
    state->jmp_map[IRBinOp::Operation::NOTEQ][1][0] = [this](uint16_t label) { this->writer->jne(this->writer->LabelById(label)); };
    state->jmp_map[IRBinOp::Operation::NOTEQ][1][1] = [this](uint16_t label) { this->writer->je(this->writer->LabelById(label)); };

}