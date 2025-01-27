#include <wind/generation/IR.h>
#include <wind/backend/writer/writer.h>
#include <functional>
#include <map>
#include <cstring>

#ifndef x86_64_BACKEND_H
#define x86_64_BACKEND_H

// -- Utils --
unsigned NearPow2(unsigned n);
unsigned align16(unsigned n);

uint8_t typeIdentify(DataType *type);
uint64_t setDesc(uint64_t desc, uint8_t index, DataType *type);
uint64_t get64bitDesc(std::vector<DataType*> &args);

Reg CastReg(Reg reg, uint8_t size);

const Reg SYSVABI_CNV[6] = {
    x86::Gp::rdi, x86::Gp::rsi, x86::Gp::rdx, x86::Gp::rcx, 
    x86::Gp::r8, x86::Gp::r9
};

const std::map<HandlerType, std::string> SYSTEM_HANDLERS = {
    {HandlerType::GUARD_HANDLER, "__WDH_guard_failed"},
    {HandlerType::BOUNDS_HANDLER, "__WDH_out_of_bounds"},
    {HandlerType::SUM_HANDLER, "__WDH_sum_overflow"},
    {HandlerType::SUB_HANDLER, "__WDH_sub_overflow"},
    {HandlerType::MUL_HANDLER, "__WDH_mul_overflow"},
    {HandlerType::DIV_HANDLER, "__WDH_div_overflow"},
    {HandlerType::CANARY_HANDLER, "__WD_canary_fail"}
};

class WindEmitter {
private:
    IRBody *program;
    Ax86_64 *writer;

    // --- Register allocator ---
    class RegisterAllocator {
    public:
        struct RegValue {
            bool isDirty=false;
            enum Lifetime {
                EXPRESSION,
                FN_CALL,
                LOOP,
                UNTIL_ALLOC,
                INDEXING
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
        void Reset();
        void PostExpression();
        void PostLoop();
        Reg *FindLocalVar(int16_t stack_offset, uint16_t size); // Find if a local is in a register
        Reg *FindLabel(std::string label, uint16_t size); // Find if a label is in a register
        bool isDirty(Reg reg) { return regs[reg.id].isDirty; }
        void SetIndexing(Reg reg) { this->SetDirty(reg); regs[reg.id].lifetime = RegValue::Lifetime::INDEXING; }
        void AllocRepr();
        RegValue GetRegState(Reg reg) { return regs[reg.id]; }
        void SetRegState(Reg reg, RegValue state) { regs[reg.id] = state; }
    } *regalloc;

    // --- Section management ---

    class BackendState {
        private:
            WindEmitter &emitter;
        public:
            // --- Section management ---

            struct Sections {
                uint16_t text;
                uint16_t data;
                uint16_t rodata;
            } sections;

            uint16_t ActiveLabel() { return emitter.writer->ActiveLabel(); }

            // --- String management ---

            struct Strings {
                std::map<std::string, std::string> strmap; // string -> label
                uint16_t str_i = 0;
            } strings;

            std::string string(std::string str);
            void EmitStrings();

            // --- Handler management ---

            struct HandlerDesc {
                enum Type {
                    USER,
                    SYSTEM
                } type;
                std::string label;
                std::string handler_fn; // for system handlers
                HandlerType op_handle;
                uint16_t handler_i;
            };

            struct ExcHandlers {
                std::map<HandlerType, HandlerDesc> active_handlers;
                std::vector<HandlerDesc> used_handlers;
            } *handlers;


            const char *RequestHandler(HandlerType type) {
                if (this->handlers->active_handlers.find(type) == this->handlers->active_handlers.end()) {
                    std::string label = ".L__"+this->fn.ref->name() + "." + std::to_string(type) + ".handler";
                    this->handlers->active_handlers[type] = {
                        .type = HandlerDesc::Type::SYSTEM,
                        .label = label,
                        .handler_fn = SYSTEM_HANDLERS.at(type),
                        .op_handle = type
                    };
                    this->handlers->used_handlers.push_back(this->handlers->active_handlers[type]);
                }
                return strdup(this->handlers->active_handlers[type].label.c_str());
            }

            void SetHandler(HandlerType type, std::string label) {
                this->handlers->active_handlers[type] = {
                    .type = HandlerDesc::Type::USER,
                    .label = label,
                    .handler_fn = "",
                    .op_handle = type
                };
            }
            void AppendHandler(HandlerDesc handler) {
                this->handlers->used_handlers.push_back(handler);
            }

            uint16_t NewHandlerFlow(HandlerType handler) {
                return this->emitter.writer->NewLabel(
                    ".L__"+std::to_string(this->l_flow_i++)+"."+std::to_string(handler)+".handler"
                );
            }

            // --- Function management ---

            struct Function {
                IRFunction *ref;

                uint16_t in_arg_i=0;
            } fn;

            void UpdateFn(IRFunction *fn) {
                this->fn.ref = fn;
                this->fn.in_arg_i = 0;
                this->handlers = new ExcHandlers();
            }

            // --- Logical management ---

            struct LogicalFlow {
                std::string start; // for loops
                std::string end; // loops and branches
            } *l_flow;
            uint16_t l_flow_i =0;

            typedef std::function<void(uint16_t)> writer_jmp_generic;
            std::map<IRBinOp::Operation, writer_jmp_generic[2][2]> jmp_map;

            uint16_t NewLogicalFlow() {
                std::string label = ".L__"+std::to_string(this->l_flow_i)+".flow";
                this->l_flow_i++;
                return emitter.writer->NewLabel(label);
            }

            // --- Constructor ---

            BackendState(WindEmitter &emitter): emitter(emitter) {
                this->sections.data = emitter.writer->NewSection(".rodata");
                this->sections.data = emitter.writer->NewSection(".data");
                this->sections.text = emitter.writer->NewSection(".text");
            }
    };

    BackendState *state;

public:
    WindEmitter(IRBody *program): program(program), writer(new Ax86_64()), regalloc(new RegisterAllocator()), state(new BackendState(*this)) {
        this->SetupJumpMap();
    }
    ~WindEmitter() {
        delete writer;
        delete regalloc;
        delete state;
    }
    void Process();
    std::string GetAsm() { return writer->Emit(); }
    std::string emitObj(std::string outpath="");

private:
    // -- state.cpp --
    void SetupJumpMap();

    // -- func.cpp --
    void ProcessFunction(IRFunction *func);
        void EmitFnPrologue(IRFunction *fn);
            void EmitFnProCanary();
        void EmitFnEpilogue();
            void EmitFnEpiCanary();
        void EmitFnStatement(IRNode *node);
    Reg EmitFnCall(IRFnCall *call, Reg dst);
        void EmitFnCallArgs(IRFnCall *call, std::vector<DataType*> &arg_types);
            void EmitFnCallStackArg(IRNode *argv);
    
    // -- vars.cpp --
    void ProcessGlobal(IRGlobalDecl *decl);
    

    // -- expr.cpp --
    Reg EmitExpr(IRNode *node, Reg dst, bool keepDstSize=false, bool isJmp=false);
    /*
        - keepDstSize: If true, the destination register will keep its size (or resize if needed)
        - isJmp: If true, the expression does not execute set conditional operations
    */
        Reg EmitBinOp(IRBinOp *expr, Reg dst, bool isJmp=false);
        Reg EmitValue(IRNode *node, Reg dst);
            Reg EmitValLiteral(IRLiteral *lit, Reg dst);
            Reg EmitStrLiteral(IRStringLiteral *str, Reg dst);
            Reg EmitTypeCast(IRTypeCast *cast, Reg dst);
            // -- vars.cpp --
            Reg EmitLocalVar(IRLocalRef *local, Reg dst);
            Reg EmitGlobalVar(IRGlobRef *global, Reg dst);
            void EmitIntoLocalVar(IRLocalRef *local, IRNode *value);
            void EmitIntoGlobalVar(IRGlobRef *global, IRNode *value);
            // -- ptr.cpp --
            Reg EmitLocalPtr(IRLocalAddrRef *local, Reg dst);
            void EmitIntoGenPtr(const IRNode *ptr, const IRNode *value);
            Reg EmitGenPtrIndex(IRGenericIndexing *ptr, Reg dst);
        template <typename srcT>
        Reg EmitArithmeticOp(IRBinOp::Operation op, Reg dst, srcT right, bool isJmp=false);
            Reg EmitOptArithmeticLVOp(IRBinOp::Operation op, Reg dst, const IRLocalRef *local, bool isJmp=false);
            Reg EmitOptArithmeticGVOp(IRBinOp::Operation op, Reg dst, const IRGlobRef *global, bool isJmp=false);

    void TryCast(Reg dst, Reg src);
    
    // -- cond.cpp --
    void EmitLoop(IRLooping *loop);
    void EmitBranch(IRBranching *branch);
        void EmitCondJump(IRNode *cmp, uint16_t label, bool invert=false);
    void EmitContinue();
    void EmitBreak();


    // -- flow.cpp --
    void EmitReturn(IRRet *ret);
    void EmitInAsm(IRInlineAsm *asmn);
    Reg EmitPtrGuard(IRPtrGuard *guard, Reg dst);
    Reg EmitFnRef(IRFnRef *fn_ref, Reg dst);
    void EmitTryCatch(IRTryCatch *trycatch);

    // -- backend.cpp --
    void ProcessStatement(IRNode *node);
    void ProcessTop(IRNode *node);

};

#endif