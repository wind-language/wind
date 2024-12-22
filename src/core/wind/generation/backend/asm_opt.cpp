#include <asmjit/asmjit.h>
#include <wind/generation/backend.h>
#include <regex>
#include <iostream>

void WindEmitter::OptVarMoved(int offset, asmjit::x86::Gp reg) {
asmjit::x86::Gp check = this->OptVarGet(offset);
  if (check != asmjit::x86::r15) {
    this->OptClearReg(check);
  }
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

std::string WindEmitter::OptCommonRed(std::string asmr) {
    // Define the regex pattern to detect redundancy
    std::regex pattern(R"(mov\s+(qword|dword|word|byte)?\s*ptr\s*\[(rbp-[^\]]+)\],\s+([a-z]+)\s*\n\s*mov\s+([a-z]+),\s+(qword|dword|word|byte)?\s*ptr\s*\[\2\])");
    
    // Use $4 and $3 for replacement based on capture groups
    std::string replacement = R"(mov [$2], $4)";
    
    // Perform the replacement
    std::string opt = std::regex_replace(asmr, pattern, replacement);
    
    return opt;
}