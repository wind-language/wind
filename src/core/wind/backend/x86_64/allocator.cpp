/**
 * @file allocator.cpp
 * @brief Implementation of the RegisterAllocator class for x86_64 backend.
 */

#include <wind/generation/IR.h>
#include <wind/backend/writer/writer.h>
#include <wind/backend/x86_64/backend.h>
#include <stdexcept>
#include <iostream>

/**
 * @brief Marks a register as dirty.
 * @param reg The register to mark as dirty.
 */
void WindEmitter::RegisterAllocator::SetDirty(Reg reg) {
    regs[reg.id].isDirty = true;
    regs[reg.id].lifetime = RegValue::EXPRESSION;
    regs[reg.id].stack_offset = 0;
    regs[reg.id].label = "";
}


/**
 * @brief Frees a register.
 * @param reg The register to free.
 */
void WindEmitter::RegisterAllocator::Free(Reg reg) {
    regs[reg.id].isDirty = false;
    regs[reg.id].lifetime = RegValue::EXPRESSION;
    regs[reg.id].stack_offset = 0;
    regs[reg.id].label = "";
}

/**
 * @brief Allocates a register.
 * @param size The size of the register.
 * @return The allocated register.
 * @throws std::runtime_error if no free registers are available.
 */
Reg WindEmitter::RegisterAllocator::Allocate(uint8_t size, bool setDirty) {
    for (uint8_t i = 0; i < 16; i++) {
        if ( (i>3 && i<6)) { continue; }
        if (regs[i].lifetime == RegValue::Lifetime::UNTIL_ALLOC || !regs[i].isDirty) {
            Reg freg = Reg({i, size, Reg::GPR});
            if (setDirty) this->SetDirty(freg);
            return freg;
        }
    }
    throw std::runtime_error("No free registers");
}

/**
 * @brief Sets a variable in a register.
 * @param reg The register.
 * @param offset The stack offset.
 * @param lifetime The lifetime of the variable.
 */
void WindEmitter::RegisterAllocator::SetVar(Reg reg, int16_t offset, RegValue::Lifetime lifetime) {
    regs[reg.id].stack_offset = offset;
    regs[reg.id].lifetime = lifetime;
    regs[reg.id].isDirty = true;
    regs[reg.id].label = "";
}

void WindEmitter::RegisterAllocator::SetLabel(Reg reg, std::string label, RegValue::Lifetime lifetime) {
    regs[reg.id].label = label;
    regs[reg.id].lifetime = lifetime;
    regs[reg.id].isDirty = true;
    regs[reg.id].stack_offset = 0;
}

void WindEmitter::RegisterAllocator::SetLifetime(Reg reg, RegValue::Lifetime lifetime) {
    regs[reg.id].lifetime = lifetime;
}

/**
 * @brief Frees registers after a function call.
 */
void WindEmitter::RegisterAllocator::Reset() {
    // Every register is compromised after a function call
    for (uint8_t i = 0; i < 16; i++) {
        this->Free((Reg){i, 8, Reg::GPR});
    }
}

/**
 * @brief Frees registers after an expression.
 */
void WindEmitter::RegisterAllocator::PostExpression() {
    for (uint8_t i = 0; i < 16; i++) {
        if (regs[i].isDirty && regs[i].lifetime == RegValue::EXPRESSION) {
            this->Free((Reg){i, 8, Reg::GPR});
        }
    }
}

/**
 * @brief Frees registers after a loop.
 */
void WindEmitter::RegisterAllocator::PostLoop() {
    for (uint8_t i = 0; i < 16; i++) {
        if (regs[i].lifetime == RegValue::LOOP) {
            this->Free((Reg){i, 8, Reg::GPR});
        }
    }
}

/**
 * @brief Finds a local variable in the registers.
 * @param offset The stack offset of the variable.
 * @return The register containing the variable, or nullptr if not found.
 */
Reg *WindEmitter::RegisterAllocator::FindLocalVar(int16_t offset, uint16_t size) {
    for (uint8_t i = 0; i < 16; i++) {
        if (regs[i].isDirty && regs[i].stack_offset == offset) {
            Reg freg = Reg({i, (uint8_t)size, Reg::GPR});
            return new Reg(freg);
        }
    }
    return nullptr;
}

Reg *WindEmitter::RegisterAllocator::FindLabel(std::string label, uint16_t size) {
    for (uint8_t i = 0; i < 16; i++) {
        if (regs[i].isDirty && regs[i].label == label) {
            Reg freg = Reg({i, (uint8_t)size, Reg::GPR});
            return new Reg(freg);
        }
    }
    return nullptr;
}

/**
 * @brief Requests a register.
 * @param reg The register to request.
 * @return True if the register is available, false otherwise.
 */
bool WindEmitter::RegisterAllocator::Request(Reg reg) {
    if (regs[reg.id].lifetime == RegValue::UNTIL_ALLOC) {
        return true;
    }
    else if (regs[reg.id].isDirty) {
        return false;
    }
    return true;
}


const std::string GP_REG_MAP[16][4] = {
    {"al", "ax", "eax", "rax"},
    {"bl", "bx", "ebx", "rbx"},
    {"cl", "cx", "ecx", "rcx"},
    {"dl", "dx", "edx", "rdx"},
    {"ah", "sp", "esp", "rsp"},
    {"ch", "bp", "ebp", "rbp"},
    {"dh", "si", "esi", "rsi"},
    {"bh", "di", "edi", "rdi"},
    {"r8b", "r8w", "r8d", "r08"},
    {"r9b", "r9w", "r9d", "r09"},
    {"r10b", "r10w", "r10d", "r10"},
    {"r11b", "r11w", "r11d", "r11"},
    {"r12b", "r12w", "r12d", "r12"},
    {"r13b", "r13w", "r13d", "r13"},
    {"r14b", "r14w", "r14d", "r14"},
    {"r15b", "r15w", "r15d", "r15"}
};
const std::string LIFETIME_MAP[4] = {
    "EXPR",
    "FN_CALL",
    "LOOP",
    "UNTIL_ALLOC"
};

void WindEmitter::RegisterAllocator::AllocRepr() {
    for (uint8_t i=0;i<16;i++) {
        Reg reg = {i, 8, Reg::GPR};
        std::cerr <<
            "[" << GP_REG_MAP[reg.id][__builtin_ctz(reg.size)] << "] " <<
            (regs[i].isDirty ? "DIRTY" : "CLEAN") <<
            " (" << regs[i].stack_offset << ") (" << regs[i].label << ") " << LIFETIME_MAP[regs[i].lifetime] << std::endl;
    }
}