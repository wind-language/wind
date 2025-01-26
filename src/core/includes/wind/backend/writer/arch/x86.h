#include <wind/backend/writer/writer.h>
#include <iostream>
#ifndef WRITER_Ax86_64_H
#define WRITER_Ax86_64_H

#define GPREG64(name, id) const Reg name = {id, 8, Reg::GPR, true}
#define GPREG32(name, id) const Reg name = {id, 4, Reg::GPR, true}
#define GPREG16(name, id) const Reg name = {id, 2, Reg::GPR, true}
#define GPREG8(name, id) const Reg name = {id, 1, Reg::GPR, true}
#define SEGREG(name, id) const Reg name = {id, 8, Reg::SEG, true}

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

#define REG_INSTR_HANDLE(handler, reg) \
    if (handler != 0 && reg.id != x86::Gp::rsp.id && OverflowChecks) { \
        if (reg.signed_value) \
            this->Write("jo", handler); \
        else \
            this->Write("jc", handler); \
    }

#define MEM_INSTR_HANDLE(handler) \
    if (handler != 0 && OverflowChecks) { \
        this->Write("jc", handler); \
    }


#define A_IRR_INSTR(name) void name(Reg dst, Reg src, const char *handler=nullptr) { this->Write(#name, dst, src); REG_INSTR_HANDLE(handler, dst) } // IRR stands for "Instruction Register-Register"
#define A_IRM_INSTR(name) void name(Reg dst, Mem src, const char *handler=nullptr) { this->Write(#name, dst, src); REG_INSTR_HANDLE(handler, dst) } // IRM stands for "Instruction Register-Memory"
#define A_IMR_INSTR(name) void name(Mem dst, Reg src, const char *handler=nullptr) { this->Write(#name, dst, src);  REG_INSTR_HANDLE(handler, src) } // IMR stands for "Instruction Memory-Register"
#define A_IRI_INSTR(name) void name(Reg dst, int64_t imm, const char *handler=nullptr) { this->Write(#name, dst, imm); REG_INSTR_HANDLE(handler, dst) } // IRI stands for "Instruction Register-Immediate"
#define A_IMI_INSTR(name) void name(Mem dst, int64_t imm, const char *handler=nullptr) { this->Write(#name, dst, imm); MEM_INSTR_HANDLE(handler) } // IMI stands for "Instruction Memory-Immediate"
#define A_IIR_INSTR(name) void name(int64_t imm, Reg src, const char *handler=nullptr) { this->Write(#name, imm, src); } // IIR stands for "Instruction Immediate-Register"
#define A_RROI_INSTR(name) void name(Reg src, RegOffset offs, const char *handler=nullptr) { this->Write(#name, src, offs); } // RROI stands for "Register Register Offset"
#define A_IIM_INSTR(name) void name(int64_t imm, Mem src, const char *handler=nullptr) { this->Write(#name, imm, src); } // IIM stands for "Instruction Immediate-Memory"
#define A_FIVE_INSTR(name) \
    A_IRR_INSTR(name) \
    A_IRM_INSTR(name) \
    A_IMR_INSTR(name) \
    A_IRI_INSTR(name) \
    A_IMI_INSTR(name) // FIVE for (IRR IRM IMR)

#define A_IMM_EX_INSTR(name) void name(Mem dst, Mem src, const char *handler=nullptr) { this->Write(#name, dst, src); MEM_INSTR_HANDLE(handler) } // IMM_EX stands for "Instruction Memory-Memory Extra"


#define B_N_INSTR(name) void name() { this->Write(#name); } // N stands for "No"
#define B_IR_INSTR(name) void name(Reg dst) { this->Write(#name, dst); } // IR stands for "Instruction Register"
#define B_IM_INSTR(name) void name(Mem dst) { this->Write(#name, dst); } // IM stands for "Instruction Memory"
#define B_IL_INSTR(name) void name(std::string label) { this->Write(#name, label); } // IL stands for "Instruction Label"
#define B_II_INSTR(name) void name(int64_t imm) { this->Write(#name, imm); } // II stands for "Instruction Immediate"
#define B_QUAD_INSTR(name) \
    B_IR_INSTR(name) \
    B_IM_INSTR(name) \
    B_II_INSTR(name) \
    B_IL_INSTR(name) // TRIPLE for (IR IM IL)


#define C_SEVEN_INSTR(name) \
    A_IRR_INSTR(name) \
    A_IRM_INSTR(name) \
    A_IMR_INSTR(name) \
    A_IRI_INSTR(name) \
    A_IMI_INSTR(name) \
    A_IIR_INSTR(name) \
    A_RROI_INSTR(name) \
    A_IIM_INSTR(name) // SEVEN for (IRR IRM IMR IRI IMI IIR IIM)
    
// ----------------------------


#define SPECIAL_ARITHMETIC(name)\
    void name(Reg src, const char *handler=nullptr) { this->Write(#name, src); REG_INSTR_HANDLE(handler, src) } \
    void name(Mem src, const char *handler=nullptr) { this->Write(#name, src); MEM_INSTR_HANDLE(handler) } \
    void name(int64_t imm, const char *handler=nullptr) { this->Write(#name, imm); }


class Ax86_64 : public WindWriter {
private:
    // Resolve regs
    std::string ResolveGpr(Reg &reg);
    std::string ResolveSeg(Reg &reg);
    std::string ResolveReg(Reg &reg) override;
    std::string ResolveRegOff(RegOffset &offs) override;
    std::string ResolveMem(Mem &reg) override;
    std::string ResolveWord(uint16_t size) override;

public:
    bool OverflowChecks = true;
    Ax86_64() {}
    
    A_FIVE_INSTR(mov)
    A_FIVE_INSTR(lea)
    A_IMM_EX_INSTR(lea)
    A_FIVE_INSTR(add)
    A_FIVE_INSTR(sub)
    A_FIVE_INSTR(shr)
    A_FIVE_INSTR(shl)
    A_FIVE_INSTR(sar)
    A_FIVE_INSTR(sal)
    A_FIVE_INSTR(imul)
    A_FIVE_INSTR(mul)

    A_FIVE_INSTR(inc)
    A_FIVE_INSTR(dec)

    SPECIAL_ARITHMETIC(div)
    SPECIAL_ARITHMETIC(idiv)


    A_FIVE_INSTR(movzx)
    A_FIVE_INSTR(movsx)

    B_QUAD_INSTR(jmp)
    B_QUAD_INSTR(call)
    B_QUAD_INSTR(push)
    B_QUAD_INSTR(pop)
    B_QUAD_INSTR(je)
    B_QUAD_INSTR(jz)
    B_QUAD_INSTR(jne)
    B_QUAD_INSTR(jg)
    B_QUAD_INSTR(jge)
    B_QUAD_INSTR(jl)
    B_QUAD_INSTR(jle)
    B_QUAD_INSTR(ja)
    B_QUAD_INSTR(jae)
    B_QUAD_INSTR(jb)
    B_QUAD_INSTR(jbe)
    B_QUAD_INSTR(jnb)
    B_QUAD_INSTR(jo)
    B_QUAD_INSTR(jno)
    B_QUAD_INSTR(js)
    B_QUAD_INSTR(jns)
    B_QUAD_INSTR(jp)
    B_QUAD_INSTR(jnp)
    B_QUAD_INSTR(sete)
    B_QUAD_INSTR(setne)
    B_QUAD_INSTR(setg)
    B_QUAD_INSTR(setge)
    B_QUAD_INSTR(setl)
    B_QUAD_INSTR(setle)
    B_QUAD_INSTR(seta)
    B_QUAD_INSTR(setae)
    B_QUAD_INSTR(setb)
    B_QUAD_INSTR(setbe)

    B_N_INSTR(leave)
    B_N_INSTR(ret)
    B_N_INSTR(rdtsc)
    B_N_INSTR(rdtscp)
    B_N_INSTR(cdq)
    B_N_INSTR(cqo)
    B_N_INSTR(cdqe)

    C_SEVEN_INSTR(cmp)
    C_SEVEN_INSTR(test)



    inline void and_(Reg dst, Reg src) { this->Write("and", dst, src); }
    inline void and_(Reg dst, int32_t imm) { this->Write("and", dst, imm); }
    inline void and_(Mem mem, Reg src) { this->Write("and", mem, src); }
    inline void and_(Mem mem, int32_t imm) { this->Write("and", mem, imm); }
    inline void and_(Reg dst, Mem mem) { this->Write("and", dst, mem); }

    inline void or_(Reg dst, Reg src) { this->Write("or", dst, src); }
    inline void or_(Reg dst, int32_t imm) { this->Write("or", dst, imm); }
    inline void or_(Mem mem, Reg src) { this->Write("or", mem, src); }
    inline void or_(Mem mem, int32_t imm) { this->Write("or", mem, imm); }
    inline void or_(Reg dst, Mem mem) { this->Write("or", dst, mem); }

    inline void xor_(Reg dst, Reg src) { this->Write("xor", dst, src); }
    inline void xor_(Reg dst, int32_t imm) { this->Write("xor", dst, imm); }
    inline void xor_(Mem mem, Reg src) { this->Write("xor", mem, src); }
    inline void xor_(Mem mem, int32_t imm) { this->Write("xor", mem, imm); }
    inline void xor_(Reg dst, Mem mem) { this->Write("xor", dst, mem); }
};

#endif