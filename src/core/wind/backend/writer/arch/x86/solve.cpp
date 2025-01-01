#include <wind/backend/writer/writer.h>
#include <iostream>
#include <cmath>

const std::string GP_REG_MAP[16][4] = {
    {"al", "ax", "eax", "rax"},
    {"bl", "bx", "ebx", "rbx"},
    {"cl", "cx", "ecx", "rcx"},
    {"dl", "dx", "edx", "rdx"},
    {"ah", "sp", "esp", "rsp"},
    {"ch", "bp", "ebp", "rbp"},
    {"dh", "si", "esi", "rsi"},
    {"bh", "di", "edi", "rdi"},
    {"r8b", "r8w", "r8d", "r8"},
    {"r9b", "r9w", "r9d", "r9"},
    {"r10b", "r10w", "r10d", "r10"},
    {"r11b", "r11w", "r11d", "r11"},
    {"r12b", "r12w", "r12d", "r12"},
    {"r13b", "r13w", "r13d", "r13"},
    {"r14b", "r14w", "r14d", "r14"},
    {"r15b", "r15w", "r15d", "r15"}
};

const std::string SEG_REG_MAP[8] = {
    "es", "cs", "ss", "ds", "fs", "gs"
};

std::string Ax86_64::ResolveGpr(Reg &reg) {
    return GP_REG_MAP[reg.id][__builtin_ctz(reg.size)];
}

std::string Ax86_64::ResolveSeg(Reg &reg) {
    return SEG_REG_MAP[reg.id];
}

std::string Ax86_64::ResolveReg(Reg &reg) {
    switch (reg.type) {
        case Reg::GPR:
            return ResolveGpr(reg);
        case Reg::SEG:
            return ResolveSeg(reg);
        default:
            return "";
    }
}

std::string Ax86_64::ResolveWord(uint16_t size) {
    switch (size) {
        case 1:
            return "byte";
        case 2:
            return "word";
        case 4:
            return "dword";
        default:
            return "qword";
    }
}

std::string Ax86_64::ResolveMem(Mem &mem) {
    std::string res = this->ResolveWord(mem.size) + " ptr [";
    switch (mem.base_type) {
        case Mem::BASE:
            res += ResolveReg(mem.base);
            break;
        case Mem::LABEL : {
            res += mem.label;
            break;
        }
    }
    switch (mem.offset_type) {
        case Mem::IMM:
            if(mem.offset!=0){ res += (mem.offset < 0 ? "-" : "+") + std::to_string(std::abs(mem.offset)); }
            break;
        case Mem::REG:
            res += " + " + ResolveReg(mem.index);
            break;
    }
    res += "]";
    return res;
}