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
}


/**
 * @brief Frees a register.
 * @param reg The register to free.
 */
void WindEmitter::RegisterAllocator::Free(Reg reg) {
    regs[reg.id].isDirty = false;
    regs[reg.id].lifetime = RegValue::EXPRESSION;
    regs[reg.id].stack_offset = 0;
}

/**
 * @brief Allocates a register.
 * @param size The size of the register.
 * @return The allocated register.
 * @throws std::runtime_error if no free registers are available.
 */
Reg WindEmitter::RegisterAllocator::Allocate(uint8_t size, bool setDirty) {
    for (uint8_t i = 0; i < 16; i++) {
        if ( (i>5 && i<8) || i==1 ) { continue; }
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
void WindEmitter::RegisterAllocator::SetVar(Reg reg, uint16_t offset, RegValue::Lifetime lifetime) {
    regs[reg.id].stack_offset = offset;
    regs[reg.id].lifetime = lifetime;
    regs[reg.id].isDirty = true;
}

/**
 * @brief Frees registers after a function call.
 */
void WindEmitter::RegisterAllocator::PostCall() {
    for (uint8_t i = 0; i < 16; i++) {
        if (regs[i].lifetime == RegValue::FN_CALL) {
            this->Free((Reg){i, 8, Reg::GPR});
        }
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
Reg *WindEmitter::RegisterAllocator::FindLocalVar(uint16_t offset) {
    for (uint8_t i = 0; i < 16; i++) {
        if (regs[i].isDirty && regs[i].stack_offset == offset) {
            Reg freg = Reg({i, 8, Reg::GPR});
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
    this->SetDirty(reg);
    return true;
}