#include <wind/backend/writer/writer.h>
#include <iostream>

const std::string GP_REG_MAP[64] = {
    "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh",
    "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b",
    "ax", "cx", "dx", "bx", "sp", "bp", "si", "di",
    "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w",
    "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d",
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
};

const std::string SEG_REG_MAP[8] = {
    "es", "cs", "ss", "ds", "fs", "gs"
};

std::string Ax86_64::ResolveGpr(Reg &reg) {
    return GP_REG_MAP[
        reg.id + 16*__builtin_ctz(reg.size)
    ];
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
            res += (mem.offset < 0 ? "-" : "+") + std::to_string(std::abs(mem.offset));
            break;
        case Mem::REG:
            res += " + " + ResolveReg(mem.index);
            break;
    }
    res += "]";
    return res;
}