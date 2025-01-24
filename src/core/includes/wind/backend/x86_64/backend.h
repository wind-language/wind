#include <wind/generation/IR.h>
#include <wind/backend/writer/writer.h>
#include <functional>
#include <map>
#include <cstring>

#ifndef x86_64_BACKEND_H
#define x86_64_BACKEND_H

#define HANDLER_LABEL(fn_name, op) \
    (".L__"+fn_name+"_"+op+".handler")

struct BaseHandlerDesc {
    bool needEmit;
    const char *handler_fn;
};
struct UserHandlerDesc {
    const char *handler_label;
    uint16_t lj_label_callback;
    IRBody *body;
    std::map<std::string, UserHandlerDesc> handler_ctx;
};

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

    struct FlowDesc {
        uint16_t start;
        uint16_t end;
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
            int16_t stack_offset; // holding local stack offset
            std::string label;     // holding label address
        };
        RegValue regs[16]; // 16 GP registers
        RegisterAllocator() {}
        ~RegisterAllocator() {}
        void SetDirty(Reg reg); // Mark a register as dirty, all sizes
        Reg Allocate(uint8_t size, bool setDirty=true); // Find a free register
        bool Request(Reg reg); // Request a specific register
        void SetVar(Reg reg, int16_t stack_offset, RegValue::Lifetime lifetime); // Set a local to a register
        void SetLabel(Reg reg, std::string label, RegValue::Lifetime lifetime); // Set a label to a register
        void SetLifetime(Reg reg, RegValue::Lifetime lifetime); // Set a lifetime to a register
        void Free(Reg reg); // Free a register
        void FreeAllRegs();
        void PostExpression();
        void PostLoop();
        Reg *FindLocalVar(int16_t stack_offset, uint16_t size); // Find if a local is in a register
        Reg *FindLabel(std::string label, uint16_t size); // Find if a label is in a register
        bool isDirty(Reg reg) { return regs[reg.id].isDirty; }
        void AllocRepr();
    } regalloc;

public:
    WindEmitter(IRBody *program): program(program), writer(new Ax86_64()) {
        jmp_map[IRBinOp::Operation::EQ][0][0] = [this](uint16_t label) { this->writer->je(this->writer->LabelById(label)); };
        jmp_map[IRBinOp::Operation::EQ][0][1] = [this](uint16_t label) { this->writer->jne(this->writer->LabelById(label)); };
        jmp_map[IRBinOp::Operation::EQ][1][0] = [this](uint16_t label) { this->writer->je(this->writer->LabelById(label)); };
        jmp_map[IRBinOp::Operation::EQ][1][1] = [this](uint16_t label) { this->writer->jne(this->writer->LabelById(label)); };

        jmp_map[IRBinOp::Operation::LESS][0][0] = [this](uint16_t label) { this->writer->jb(this->writer->LabelById(label)); };
        jmp_map[IRBinOp::Operation::LESS][0][1] = [this](uint16_t label) { this->writer->jnb(this->writer->LabelById(label)); };
        jmp_map[IRBinOp::Operation::LESS][1][0] = [this](uint16_t label) { this->writer->jl(this->writer->LabelById(label)); };
        jmp_map[IRBinOp::Operation::LESS][1][1] = [this](uint16_t label) { this->writer->jge(this->writer->LabelById(label)); };

        jmp_map[IRBinOp::Operation::GREATER][0][0] = [this](uint16_t label) { this->writer->ja(this->writer->LabelById(label)); };
        jmp_map[IRBinOp::Operation::GREATER][0][1] = [this](uint16_t label) { this->writer->jbe(this->writer->LabelById(label)); };
        jmp_map[IRBinOp::Operation::GREATER][1][0] = [this](uint16_t label) { this->writer->jg(this->writer->LabelById(label)); };
        jmp_map[IRBinOp::Operation::GREATER][1][1] = [this](uint16_t label) { this->writer->jle(this->writer->LabelById(label)); };

        jmp_map[IRBinOp::Operation::LESSEQ][0][0] = [this](uint16_t label) { this->writer->jbe(this->writer->LabelById(label)); };
        jmp_map[IRBinOp::Operation::LESSEQ][0][1] = [this](uint16_t label) { this->writer->ja(this->writer->LabelById(label)); };
        jmp_map[IRBinOp::Operation::LESSEQ][1][0] = [this](uint16_t label) { this->writer->jle(this->writer->LabelById(label)); };
        jmp_map[IRBinOp::Operation::LESSEQ][1][1] = [this](uint16_t label) { this->writer->jg(this->writer->LabelById(label)); };

        jmp_map[IRBinOp::Operation::GREATEREQ][0][0] = [this](uint16_t label) { this->writer->jae(this->writer->LabelById(label)); };
        jmp_map[IRBinOp::Operation::GREATEREQ][0][1] = [this](uint16_t label) { this->writer->jb(this->writer->LabelById(label)); };
        jmp_map[IRBinOp::Operation::GREATEREQ][1][0] = [this](uint16_t label) { this->writer->jge(this->writer->LabelById(label)); };
        jmp_map[IRBinOp::Operation::GREATEREQ][1][1] = [this](uint16_t label) { this->writer->jl(this->writer->LabelById(label)); };

        jmp_map[IRBinOp::Operation::NOTEQ][0][0] = [this](uint16_t label) { this->writer->jne(this->writer->LabelById(label)); };
        jmp_map[IRBinOp::Operation::NOTEQ][0][1] = [this](uint16_t label) { this->writer->je(this->writer->LabelById(label)); };
        jmp_map[IRBinOp::Operation::NOTEQ][1][0] = [this](uint16_t label) { this->writer->jne(this->writer->LabelById(label)); };
        jmp_map[IRBinOp::Operation::NOTEQ][1][1] = [this](uint16_t label) { this->writer->je(this->writer->LabelById(label)); };
    }
    ~WindEmitter() {
        delete writer;
        delete c_flow_desc;
    }
    void Process();
    std::string GetAsm() { return writer->Emit(); }
    std::string emitObj(std::string outpath="");

private:
    void EmitFnPrologue(IRFunction *fn);
    void EmitFnEpilogue();

    void ProcessFunction(IRFunction *func);
    void ProcessGlobalDecl(IRGlobalDecl *decl);

    void EmitCJump(IRNode *node, uint16_t label, bool invert);

    void EmitReturn(IRRet *ret);
    void EmitLocDecl(IRVariableDecl *decl);
    void EmitInAsm(IRInlineAsm *asmn);
    void EmitBranch(IRBranching *branch);
    void EmitLoop(IRLooping *loop);

    void EmitTryCatch(IRTryCatch *trycatch);

    Reg EmitFnCall(IRFnCall *call, Reg dst);

    Reg EmitValue(IRNode *value, Reg dst);
    Reg EmitBinOp(IRBinOp *binop, Reg dst, bool isJmp);
    Reg EmitExpr(IRNode *expr, Reg dst, bool isJmp=false);
    Reg EmitLocRef(IRLocalRef *ref, Reg dst);
    Reg EmitGlobRef(IRGlobRef *ref, Reg dst);
    void EmitIntoLoc(IRLocalRef *ref, IRNode *value);
    Reg EmitString(IRStringLiteral *str, Reg dst);
    Reg EmitLocAddrRef(IRLocalAddrRef *ref, Reg dst);
    void EmitIntoLocAddrRef(IRLocalAddrRef *ref, Reg src);
    Reg EmitFnRef(IRFnRef *ref, Reg dst);
    Reg EmitGenAddrRef(IRGenericIndexing *ref, Reg dst);
    Reg EmitPtrGuard(IRPtrGuard *guard, Reg dst);
    Reg EmitTypeCast(IRTypeCast *cast, Reg dst);
    void EmitIntoGenAddrRef(IRGenericIndexing *ref, Reg src);

    // --- argpush utils ---
    uint8_t typeIdentify(DataType *type);
    uint64_t setDesc(uint64_t desc, uint8_t index, DataType *type);
    uint64_t get64bitDesc(std::vector<DataType *> &args);

    void ProcessStatement(IRNode *node);
    void ProcessTop(IRNode *node);

    Reg CastReg(Reg reg, uint8_t size);
    void TryCast(Reg dst, Reg proc);

    typedef std::function<void(uint16_t)> writer_jmp_generic;
    std::map<IRBinOp::Operation, writer_jmp_generic[2][2]> jmp_map;

    struct FunctionDesc {
        IRFunction *fn;
        uint16_t metadata_l=0;
        std::map<std::string, BaseHandlerDesc> base_handlers = {
            {"add", {false, "__WDH_sum_overflow"}},
            {"sub", {false, "__WDH_sub_overflow"}},
            {"mul", {false, "__WDH_mul_overflow"}},
            {"imul", {false, "__WDH_mul_overflow"}},
            {"div", {false, "__WDH_div_overflow"}},
            {"idiv", {false, "__WDH_div_overflow"}},
            {"bounds", {false, "__WDH_out_of_bounds"}},
            {"guard", {false, "__WDH_guard_failed"}}
        };
        std::map<std::string, UserHandlerDesc> active_handlers;
        std::vector<UserHandlerDesc> user_handlers;
    } *current_fn=nullptr;

    const char *GetHandlerLabel(std::string instruction) {
        if (current_fn->active_handlers.find(instruction) != current_fn->active_handlers.end()) {
            return current_fn->active_handlers[instruction].handler_label;
        }
        if (current_fn->base_handlers.find(instruction) != current_fn->base_handlers.end()) {
            current_fn->base_handlers[instruction].needEmit = true;
            std::string handler_str = HANDLER_LABEL(current_fn->fn->fn_name, instruction);
            return strdup(handler_str.c_str());
        }
        return "";
    }

};

const std::map<HandlerType, std::vector<std::string>> HANDLER_INSTR_MAP = {
    {HandlerType::SUM_HANDLER, {"add"}},
    {HandlerType::SUB_HANDLER, {"sub"}},
    {HandlerType::MUL_HANDLER, {"mul", "imul"}},
    {HandlerType::DIV_HANDLER, {"div", "idiv"}},
    {HandlerType::BOUNDS_HANDLER, {"bounds"}},
    {HandlerType::GUARD_HANDLER, {"guard"}}
};

#endif