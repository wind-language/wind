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
    /*
    Optimize redundant code in assembly output, like:
        mov rax, rax
    and especially:
        mov qword ptr [rbp-0x8], rax
        mov rax, qword ptr [rbp-0x8]
    */

    // you don't have access to the asmjit::x86::Assembler here or to IR
    // pure string manipulation
    std::vector<std::string> lines;
    std::stringstream ss(asmr);
    std::string line;
    
    while (std::getline(ss, line)) {
        lines.push_back(line);
    }

    std::vector<std::string> optimized;
    
    for (size_t i = 0; i < lines.size(); i++) {
        bool skip = false;
        std::string curr = lines[i];
        
        if (curr.empty()) continue;
        
        if (curr.find("mov") != std::string::npos) {
            size_t comma = curr.find(',');
            if (comma != std::string::npos) {
                std::string dest = curr.substr(curr.find("mov") + 4, comma - (curr.find("mov") + 4));
                std::string src = curr.substr(comma + 1);
                // Trim whitespace
                dest.erase(0, dest.find_first_not_of(" \t"));
                dest.erase(dest.find_last_not_of(" \t") + 1);
                src.erase(0, src.find_first_not_of(" \t"));
                src.erase(src.find_last_not_of(" \t") + 1);
                if (dest == src) continue;
            }
        }
        
        if (i < lines.size() - 1) {
            std::string next = lines[i + 1];
            if (curr.find("mov") != std::string::npos && next.find("mov") != std::string::npos) {
                size_t bracket1 = curr.find('[');
                size_t bracket2 = next.find('[');
                if (bracket1 != std::string::npos && bracket2 != std::string::npos) {
                    std::string loc1 = curr.substr(bracket1, curr.find(']') - bracket1 + 1);
                    std::string loc2 = next.substr(bracket2, next.find(']') - bracket2 + 1);
                    size_t reg1_start = curr.find_last_of(' ') + 1;
                    size_t reg2_start = next.find_last_of(' ') + 1;
                    std::string reg1 = curr.substr(reg1_start);
                    std::string reg2 = next.substr(reg2_start);
                    if (loc1 == loc2 && reg1 == reg2) {
                        skip = false;
                        i+=2;
                    }
                }
            }
        }
        
        if (!skip) {
            optimized.push_back(curr);
        }
    }
    
    std::stringstream result;
    for (const auto& line : optimized) {
        result << line << '\n';
    }
    
    return result.str();
}