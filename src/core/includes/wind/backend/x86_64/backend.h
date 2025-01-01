#include <wind/generation/IR.h>
#include <wind/backend/writer/writer.h>
#include <map>

#ifndef x86_64_BACKEND_H
#define x86_64_BACKEND_H

class WindEmitter {
private:
    IRBody *program;
    Ax86_64 *writer;

    // ----

    uint16_t rostr_i=0; // rodata string index
    uint16_t ljl_i=0;   // logical jump label index
    std::vector<std::string> rostrs;

    // ----

    uint8_t textSection;
    uint8_t dataSection;
    uint8_t rodataSection;

    // ----

    IRFunction *current_fn;

    // ----
    
    class RegisterAllocator {
    public:
        struct RegValue {
            bool isDirty=false;
            enum Lifetime {
                EXPRESSION,
                FN_CALL,
                LOOP,
                UNTIL_ALLOC
            } lifetime;
            uint16_t stack_offset;
        };
        RegValue regs[16]; // 16 GP registers
        RegisterAllocator() {}
        ~RegisterAllocator() {}
        void SetDirty(Reg reg); // Mark a register as dirty, all sizes
        Reg Allocate(uint8_t size, bool setDirty=true); // Find a free register
        bool Request(Reg reg); // Request a specific register
        void SetVar(Reg reg, uint16_t stack_offset, RegValue::Lifetime lifetime); // Set a local to a register
        void Free(Reg reg); // Free a register
        void PostCall();
        void PostExpression();
        void PostLoop();
        Reg *FindLocalVar(uint16_t stack_offset, uint16_t size); // Find if a local is in a register
        void AllocRepr();
    } regalloc;

public:
    WindEmitter(IRBody *program): program(program), writer(new Ax86_64()) {}
    ~WindEmitter() { delete writer; }
    void Process();
    std::string GetAsm() { return writer->Emit(); }
    std::string emitObj(std::string outpath="");

private:
    void EmitFnPrologue(IRFunction *fn);
    void EmitFnEpilogue();

    void ProcessFunction(IRFunction *func);
    void ProcessGlobalDecl(IRGlobalDecl *decl);

    void EmitReturn(IRRet *ret);
    void EmitLocDecl(IRVariableDecl *decl);
    void EmitInAsm(IRInlineAsm *asmn);

    void EmitFnCall(IRFnCall *call, Reg dst);

    void EmitValue(IRNode *value, Reg dst);
    void EmitBinOp(IRBinOp *binop, Reg dst);
    void EmitExpr(IRNode *expr, Reg dst);
    void EmitLocRef(IRLocalRef *ref, Reg dst);
    void EmitGlobRef(IRGlobRef *ref, Reg dst);
    void EmitIntoLoc(IRLocalRef *ref, IRNode *value);
    void EmitString(IRStringLiteral *str, Reg dst);
    void EmitLocAddrRef(IRLocalAddrRef *ref, Reg dst);

    void ProcessStatement(IRNode *node);
    void ProcessTop(IRNode *node);

    Reg CastReg(Reg reg, uint8_t size);
};

#endif