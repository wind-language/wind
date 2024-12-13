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