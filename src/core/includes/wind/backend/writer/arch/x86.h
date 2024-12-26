#include <wind/backend/writer/writer.h>
#ifndef WRITER_Ax86_64_H
#define WRITER_Ax86_64_H

#define GPREG64(name, id) const Reg name = {id, 8, Reg::GPR}
#define GPREG32(name, id) const Reg name = {id, 4, Reg::GPR}
#define GPREG16(name, id) const Reg name = {id, 2, Reg::GPR}
#define GPREG8(name, id) const Reg name = {id, 1, Reg::GPR}
#define SEGREG(name, id) const Reg name = {id, 8, Reg::SEG}

namespace x86 {
    namespace Gp {
        GPREG64(rax, 0);
        GPREG64(rcx, 1);
        GPREG64(rdx, 2);
        GPREG64(rbx, 3);
        GPREG64(rsp, 4);
        GPREG64(rbp, 5);
        GPREG64(rsi, 6);
        GPREG64(rdi, 7);
        GPREG64(r8, 8);
        GPREG64(r9, 9);
        GPREG64(r10, 10);
        GPREG64(r11, 11);
        GPREG64(r12, 12);
        GPREG64(r13, 13);
        GPREG64(r14, 14);
        GPREG64(r15, 15);

        GPREG32(eax, 0);
        GPREG32(ecx, 1);
        GPREG32(edx, 2);
        GPREG32(ebx, 3);
        GPREG32(esp, 4);
        GPREG32(ebp, 5);
        GPREG32(esi, 6);
        GPREG32(edi, 7);
        GPREG32(r8d, 8);
        GPREG32(r9d, 9);
        GPREG32(r10d, 10);
        GPREG32(r11d, 11);
        GPREG32(r12d, 12);
        GPREG32(r13d, 13);
        GPREG32(r14d, 14);
        GPREG32(r15d, 15);

        GPREG16(ax, 0);
        GPREG16(cx, 1);
        GPREG16(dx, 2);
        GPREG16(bx, 3);
        GPREG16(sp, 4);
        GPREG16(bp, 5);
        GPREG16(si, 6);
        GPREG16(di, 7);
        GPREG16(r8w, 8);
        GPREG16(r9w, 9);
        GPREG16(r10w, 10);
        GPREG16(r11w, 11);
        GPREG16(r12w, 12);
        GPREG16(r13w, 13);
        GPREG16(r14w, 14);
        GPREG16(r15w, 15);

        GPREG8(al, 0);
        GPREG8(cl, 1);
        GPREG8(dl, 2);
        GPREG8(bl, 3);
        GPREG8(ah, 4);
        GPREG8(ch, 5);
        GPREG8(dh, 6);
        GPREG8(bh, 7);
        GPREG8(r8b, 8);
        GPREG8(r9b, 9);
        GPREG8(r10b, 10);
        GPREG8(r11b, 11);
        GPREG8(r12b, 12);
        GPREG8(r13b, 13);
        GPREG8(r14b, 14);
        GPREG8(r15b, 15);
    }
    namespace Seg {
        SEGREG(es, 0);
        SEGREG(cs, 1);
        SEGREG(ss, 2);
        SEGREG(ds, 3);
        SEGREG(fs, 4);
        SEGREG(gs, 5);
    }
    using Mem = Mem;
}


// Declaration macros

#define A_IRR_INSTRD(name) void name(Reg dst, Reg src); // IRR stands for "Instruction Register-Register"
#define A_IRM_INSTRD(name) void name(Reg dst, Mem src); // IRM stands for "Instruction Register-Memory"
#define A_IMR_INSTRD(name) void name(Mem dst, Reg src); // IMR stands for "Instruction Memory-Register"
#define A_IRI_INSTRD(name) void name(Reg dst, int64_t imm); // IRI stands for "Instruction Register-Immediate"
#define A_IMI_INSTRD(name) void name(Mem dst, int64_t imm); // IMI stands for "Instruction Memory-Immediate"
#define A_FIVE_INSTRD(name) A_IRR_INSTRD(name) A_IRM_INSTRD(name) A_IMR_INSTRD(name) A_IRI_INSTRD(name) A_IMI_INSTRD(name) // FIVE for (IRR IRM IMR IRI IMI)

#define B_N_INSTRD(name) void name(); // N stands for "No"
#define B_IR_INSTRD(name) void name(Reg dst); // IR stands for "Instruction Register"
#define B_IM_INSTRD(name) void name(Mem dst); // IM stands for "Instruction Memory"
#define B_IL_INSTRD(name) void name(std::string label); // IL stands for "Instruction Label"
#define B_TRIPLE_INSTRD(name) B_IR_INSTRD(name) B_IM_INSTRD(name) B_IL_INSTRD(name) // TRIPLE for (IR IM IL)

#define C_IRR_INSTRD(name) void name(Reg dst, Reg src, Reg src2); // IRR stands for "Instruction Register-Register"
#define C_IRM_INSTRD(name) void name(Reg dst, Mem src, Reg src2); // IRM stands for "Instruction Register-Memory"
#define C_IMR_INSTRD(name) void name(Mem dst, Reg src, Reg src2); // IMR stands for "Instruction Memory-Register"
#define C_IRI_INSTRD(name) void name(Reg dst, int64_t imm, Reg src); // IRI stands for "Instruction Register-Immediate"
#define C_IMI_INSTRD(name) void name(Mem dst, int64_t imm, Reg src); // IMI stands for "Instruction Memory-Immediate"
#define C_IIR_INSTRD(name) void name(Reg dst, Reg src, int64_t imm); // IIR stands for "Instruction Immediate-Register"
#define C_IIM_INSTRD(name) void name(Reg dst, Mem src, int64_t imm); // IIM stands for "Instruction Immediate-Memory"
#define C_SEVEN_INSTRD(name) C_IRR_INSTRD(name) C_IRM_INSTRD(name) C_IMR_INSTRD(name) C_IRI_INSTRD(name) C_IMI_INSTRD(name) C_IIR_INSTRD(name) C_IIM_INSTRD(name) // SEVEN for (IRR IRM IMR IRI IMI IIR IIM)

// ----------------------------


class Ax86_64 : public WindWriter {
private:
    // Resolve regs
    std::string ResolveGpr(Reg &reg);
    std::string ResolveSeg(Reg &reg);
    std::string ResolveReg(Reg &reg) override;
    std::string ResolveMem(Mem &reg) override;
    std::string ResolveWord(uint16_t size) override;

public:
    Ax86_64();
    
    A_FIVE_INSTRD(mov)
    A_FIVE_INSTRD(add)
    A_FIVE_INSTRD(sub)

    B_TRIPLE_INSTRD(jmp)
    B_TRIPLE_INSTRD(push)
    B_TRIPLE_INSTRD(pop)
    B_N_INSTRD(ret)

    C_SEVEN_INSTRD(cmp)
};

#endif