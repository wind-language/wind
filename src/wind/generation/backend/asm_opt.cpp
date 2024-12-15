#include <asmjit/asmjit.h>
#include <wind/generation/backend.h>
#include <iostream>

void WindEmitter::OptVarMoved(int offset, asmjit::x86::Gp reg) {
  this->reg_vars->insert({offset, reg});
}
asmjit::x86::Gp WindEmitter::OptVarGet(int offset) {
    for (auto &var : *this->reg_vars) {
        if (var.first == offset) {
            return var.second;
        }
    }
    return asmjit::x86::r15;
}

void WindEmitter::OptClearReg(asmjit::x86::Gp reg) {
    if (this->reg_vars == nullptr) {
        return;
    }
    for (auto it = this->reg_vars->begin(); it != this->reg_vars->end(); ) {
        if (it->second.id() == reg.id()) {
            it = this->reg_vars->erase(it);
        } else {
            ++it;
        }
    }
}

void WindEmitter::OptTabulaRasa() {
    this->reg_vars->clear();
}