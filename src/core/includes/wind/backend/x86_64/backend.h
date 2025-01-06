#include <wind/generation/IR.h>
#include <wind/backend/writer/writer.h>
#include <functional>
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

    struct FlowDesc {
        uint8_t start;
        uint8_t end;
    } *c_flow_desc = nullptr;

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
            uint16_t stack_offset; // holding local stack offset
            std::string label;     // holding label address
        };
        RegValue regs[16]; // 16 GP registers
        RegisterAllocator() {}
        ~RegisterAllocator() {}
        void SetDirty(Reg reg); // Mark a register as dirty, all sizes
        Reg Allocate(uint8_t size, bool setDirty=true); // Find a free register
        bool Request(Reg reg); // Request a specific register
        void SetVar(Reg reg, uint16_t stack_offset, RegValue::Lifetime lifetime); // Set a local to a register
        void SetLabel(Reg reg, std::string label, RegValue::Lifetime lifetime); // Set a label to a register
        void Free(Reg reg); // Free a register
        void FreeAllRegs();
        void PostExpression();
        void PostLoop();
        Reg *FindLocalVar(uint16_t stack_offset, uint16_t size); // Find if a local is in a register
        Reg *FindLabel(std::string label, uint16_t size); // Find if a label is in a register
        void AllocRepr();
    } regalloc;

public:
    WindEmitter(IRBody *program): program(program), writer(new Ax86_64()) {
        jmp_map[IRBinOp::Operation::EQ][0] = [this](uint8_t label) { this->writer->je(this->writer->LabelById(label)); };
        jmp_map[IRBinOp::Operation::EQ][1] = [this](uint8_t label) { this->writer->jne(this->writer->LabelById(label)); };
        jmp_map[IRBinOp::Operation::GREATER][0] = [this](uint8_t label) { this->writer->jg(this->writer->LabelById(label)); };
        jmp_map[IRBinOp::Operation::GREATER][1] = [this](uint8_t label) { this->writer->jle(this->writer->LabelById(label)); };
        jmp_map[IRBinOp::Operation::LESS][0] = [this](uint8_t label) { this->writer->jl(this->writer->LabelById(label)); };
        jmp_map[IRBinOp::Operation::LESS][1] = [this](uint8_t label) { this->writer->jge(this->writer->LabelById(label)); };
        jmp_map[IRBinOp::Operation::LESSEQ][0] = [this](uint8_t label) { this->writer->jle(this->writer->LabelById(label)); };
        jmp_map[IRBinOp::Operation::LESSEQ][1] = [this](uint8_t label) { this->writer->jg(this->writer->LabelById(label)); };
    }
    ~WindEmitter() { delete writer; }
    void Process();
    std::string GetAsm() { return writer->Emit(); }
    std::string emitObj(std::string outpath="");

private:
    void EmitFnPrologue(IRFunction *fn);
    void EmitFnEpilogue();

    void ProcessFunction(IRFunction *func);
    void ProcessGlobalDecl(IRGlobalDecl *decl);

    void EmitCJump(IRNode *node, uint8_t label, bool invert);

    void EmitReturn(IRRet *ret);
    void EmitLocDecl(IRVariableDecl *decl);
    void EmitInAsm(IRInlineAsm *asmn);
    void EmitBranch(IRBranching *branch);
    void EmitLoop(IRLooping *loop);

    Reg EmitFnCall(IRFnCall *call, Reg dst);

    Reg EmitValue(IRNode *value, Reg dst);
    Reg EmitBinOp(IRBinOp *binop, Reg dst, bool isJmp);
    Reg EmitExpr(IRNode *expr, Reg dst, bool isJmp=false);
    Reg EmitLocRef(IRLocalRef *ref, Reg dst);
    Reg EmitGlobRef(IRGlobRef *ref, Reg dst);
    void EmitIntoLoc(IRLocalRef *ref, IRNode *value);
    Reg EmitString(IRStringLiteral *str, Reg dst);
    Reg EmitLocAddrRef(IRLocalAddrRef *ref, Reg dst);

    void ProcessStatement(IRNode *node);
    void ProcessTop(IRNode *node);

    Reg CastReg(Reg reg, uint8_t size);
    void TryCast(Reg dst, Reg proc);

    typedef std::function<void(uint8_t)> writer_jmp_generic;
    std::map<IRBinOp::Operation, writer_jmp_generic[2]> jmp_map;
};

#endif