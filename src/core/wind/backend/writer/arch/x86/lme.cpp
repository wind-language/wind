#include <wind/backend/writer/writer.h>
#include <iostream>

#define A_IRR_INSTR(name) void Ax86_64::name(Reg dst, Reg src) { this->Write(#name, dst, src); } // IRR stands for "Instruction Register-Register"
#define A_IRM_INSTR(name) void Ax86_64::name(Reg dst, Mem src) { this->Write(#name, dst, src); } // IRM stands for "Instruction Register-Memory"
#define A_IMR_INSTR(name) void Ax86_64::name(Mem dst, Reg src) { this->Write(#name, dst, src); } // IMR stands for "Instruction Memory-Register"
#define A_IRI_INSTR(name) void Ax86_64::name(Reg dst, int64_t imm) { this->Write(#name, dst, imm); } // IRI stands for "Instruction Register-Immediate"
#define A_IMI_INSTR(name) void Ax86_64::name(Mem dst, int64_t imm) { this->Write(#name, dst, imm); } // IMI stands for "Instruction Memory-Immediate"
#define A_FIVE_INSTR(name) \
    A_IRR_INSTR(name) \
    A_IRM_INSTR(name) \
    A_IMR_INSTR(name) \
    A_IRI_INSTR(name) \
    A_IMI_INSTR(name) // FIVE for (IRR IRM IMR)

#define B_N_INSTR(name) void Ax86_64::name() { this->Write(#name); } // N stands for "No"
#define B_IR_INSTR(name) void Ax86_64::name(Reg dst) { this->Write(#name, dst); } // IR stands for "Instruction Register"
#define B_IM_INSTR(name) void Ax86_64::name(Mem dst) { this->Write(#name, dst); } // IM stands for "Instruction Memory"
#define B_IL_INSTR(name) void Ax86_64::name(std::string label) { this->Write(#name, label); } // IL stands for "Instruction Label"
#define B_TRIPLE_INSTR(name) \
    B_IR_INSTR(name) \
    B_IM_INSTR(name) \
    B_IL_INSTR(name) // TRIPLE for (IR IM IL)



Ax86_64::Ax86_64() {}

A_FIVE_INSTR(mov)
A_FIVE_INSTR(add)
A_FIVE_INSTR(sub)

B_TRIPLE_INSTR(jmp)
B_TRIPLE_INSTR(push)
B_TRIPLE_INSTR(pop)
B_N_INSTR(ret)