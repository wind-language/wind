#include <asmjit/asmjit.h>
#ifndef IR_H
#define IR_H

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <stdint.h>
#include <wind/bridge/opt_flags.h>

class IRNode {
public:
  enum class NodeType {
    RET,
    LOCAL_REF,
    BODY,
    FUNCTION,
    BIN_OP,
    LITERAL,
    STRING,
    LOCAL_DECL,
    ARG_DECL,
    FUNCTION_CALL,
    REGISTER,
    IN_ASM,
    LADDR_REF,
    BRANCH,
    LOOP
  };

  virtual ~IRNode() = default;
  virtual NodeType type() const = 0;

  template <typename T>
  bool is() const {
    return dynamic_cast<const T*>(this) != nullptr;
  }

  template <typename T>
  T* as() {
    return dynamic_cast<T*>(this);
  }

  template <typename T>
  const T* as() const {
    return dynamic_cast<const T*>(this);
  }
};

class DataType {
  private:
    DataType *array;
    uint16_t type_size;
    uint16_t capacity;
  public:
    DataType(uint16_t size) : type_size(size), capacity(1), array(nullptr) {}
    DataType(uint16_t size, uint16_t cap) : type_size(size), capacity(cap), array(nullptr) {}
    DataType(uint16_t size, DataType *arr) : type_size(size), capacity(UINT16_MAX), array(arr) {}
    DataType(uint16_t size, uint16_t cap, DataType *arr) : type_size(size), capacity(cap), array(arr) {}
    bool isArray() const { return array != nullptr; }
    uint16_t moveSize() const { if (!isArray()) return type_size; return 8; }
    uint16_t index2offset(uint16_t index) const {
      if (isArray()) { return index * type_size; }
      return index;
    }
    uint16_t memSize() const {
      uint32_t bytes = 0;
      if (!isArray()) {
        return type_size;
      }
      if (capacity != UINT16_MAX) {
        bytes = array->memSize() * capacity;
      } else {
        bytes = array->memSize() + Sizes::QWORD;
      }
      return bytes;
    }
    uint16_t rawSize() const { return type_size; }
    DataType *getArrayType() const { return array; }
    bool isVoid() const { return type_size == 0; }

    enum Sizes {
      BYTE = 1,
      WORD = 2,
      DWORD = 4,
      QWORD = 8,
      VOID = 0
    };
};


class IRRet : public IRNode {

public:
  explicit IRRet(std::unique_ptr<IRNode> v);
  IRNode* get() const;
  void set(std::unique_ptr<IRNode> v);
  NodeType type() const override { return NodeType::RET; }
  std::unique_ptr<IRNode> value;
};

class IRLocalRef : public IRNode {
  uint16_t stack_offset;
  DataType *var_type;

public:
  IRLocalRef(uint16_t stack_offset, DataType *type);
  uint16_t offset() const;
  DataType *datatype() const;
  NodeType type() const override { return NodeType::LOCAL_REF; }
};

class IRLocalAddrRef : public IRNode {
  uint16_t stack_offset;
  int16_t index;
  DataType *var_type;

public:
  IRLocalAddrRef(uint16_t stack_offset, DataType *type, int16_t index = -1);
  uint16_t offset() const;
  int16_t getIndex() const;
  bool isIndexed() const { return index != -1; }
  DataType *datatype() const;
  NodeType type() const override { return NodeType::LADDR_REF; }
};

class IRBody : public IRNode {
  std::vector<std::unique_ptr<IRNode>> statements;

public:
  explicit IRBody(std::vector<std::unique_ptr<IRNode>> s);
  const std::vector<std::unique_ptr<IRNode>>& get() const;
  void Set(int index, std::unique_ptr<IRNode> statement);
  IRBody& operator + (std::unique_ptr<IRNode> statement);
  IRBody& operator += (std::unique_ptr<IRNode> statement);
  NodeType type() const override { return NodeType::BODY; }
};

class IRFunction : public IRNode {
public:
  std::string fn_name;
  std::vector<std::unique_ptr<IRLocalRef>> fn_locals;
  std::unique_ptr<IRBody> fn_body;
  std::unordered_map<std::string, IRLocalRef*> local_table;
  uint16_t stack_size = 0;
  std::vector<uint16_t> used_offsets;
  FnFlags flags = 0;
  std::vector<DataType*> arg_types;
  bool call_sub = false;
  DataType *return_type;
  bool ignore_stack_abi=false;

public:
  explicit IRFunction(std::string name, std::vector<std::unique_ptr<IRLocalRef>> locals, std::unique_ptr<IRBody> body);
  const std::string& name() const;
  const std::vector<std::unique_ptr<IRLocalRef>>& locals() const;
  IRBody* body() const;
  void SetBody(std::unique_ptr<IRBody> body);
  bool isStack();
  bool isUsed(IRLocalRef *local);
  void occupyOffset(uint16_t offset);
  IRFunction *clone();
  void copyArgTypes(std::vector<DataType*> &types);
  DataType *GetArgType(int index);
  int ArgNum() const { return arg_types.size(); }
  bool isVariadic() const { return flags & FN_VARIADIC; }
  bool isCallSub() const { return call_sub; }

  IRLocalRef *NewLocal(std::string name, DataType *type);
  IRLocalRef* GetLocal(std::string name);


  NodeType type() const override { return NodeType::FUNCTION; }
};

class IRBinOp : public IRNode {
public:
  enum Operation {
    ADD,
    SUB,
    MUL,
    DIV,
    SHL,
    SHR,
    AND,
    EQ,
    LESS,
    GREATER,
    LESSEQ,
    ASSIGN,
    MOD,
    LOGAND
  };

private:
  std::unique_ptr<IRNode> expr_left;
  std::unique_ptr<IRNode> expr_right;
  Operation op;

public:
  IRBinOp(std::unique_ptr<IRNode> l, std::unique_ptr<IRNode> r, Operation o);
  const IRNode* left() const;
  const IRNode* right() const;
  Operation operation() const;
  NodeType type() const override { return NodeType::BIN_OP; }
};

IRBinOp::Operation IRstr2op(std::string str);

class IRLiteral : public IRNode {
  long long value;

public:
  explicit IRLiteral(long long v);
  long long get() const;
  NodeType type() const override { return NodeType::LITERAL; }
};

class IRStringLiteral : public IRNode {
  std::string value;

public:
  explicit IRStringLiteral(std::string v);
  const std::string& get() const;
  NodeType type() const override { return NodeType::STRING; }
};

class IRVariableDecl : public IRNode {
  IRLocalRef *local_ref;
  IRNode *v_value;

public:
  IRVariableDecl(IRLocalRef* local_ref, IRNode* value);
  IRLocalRef *local() const;
  IRNode *value() const;
  NodeType type() const override { return NodeType::LOCAL_DECL; }
};

class IRArgDecl : public IRNode {
  IRLocalRef *local_ref;

public:
  IRArgDecl(IRLocalRef* local_ref);
  IRLocalRef *local() const;
  NodeType type() const override { return NodeType::ARG_DECL; }
};

class IRFnCall : public IRNode {
  std::string fn_name;
  std::vector<std::unique_ptr<IRNode>> fn_args;

public:
  IRFnCall(std::string name, std::vector<std::unique_ptr<IRNode>> args);
  const std::string& name() const;
  const std::vector<std::unique_ptr<IRNode>>& args() const;
  void push_arg(std::unique_ptr<IRNode> arg);
  void replaceArg(int index, std::unique_ptr<IRNode> arg);
  NodeType type() const override { return NodeType::FUNCTION_CALL; }
};

class IRRegister : public IRNode {
  asmjit::x86::Gp v_reg;

public:
  explicit IRRegister(asmjit::x86::Gp r);
  asmjit::x86::Gp reg() const;
  NodeType type() const override { return NodeType::REGISTER; }
};

class IRInlineAsm : public IRNode {
  std::string asm_code;

public:
  explicit IRInlineAsm(std::string code);
  const std::string& code() const;
  NodeType type() const override { return NodeType::IN_ASM; }
};

struct IRBranch {
  std::unique_ptr<IRNode> condition;
  std::unique_ptr<IRNode> body;
};

class IRBranching : public IRNode {
  std::vector<IRBranch> branches;
  IRBody *else_branch;

public:
  explicit IRBranching(std::vector<IRBranch> &branches);
  const std::vector<IRBranch>& getBranches() const;
  IRBody* getElseBranch() const;
  void setElseBranch(IRBody *body);
  NodeType type() const override { return NodeType::BRANCH; }
};

class IRLooping : public IRNode {
  IRNode *condition;
  IRBody *body;

public:
  IRNode* getCondition() const;
  void setCondition(IRNode* c);
  IRBody* getBody() const;
  void setBody(IRBody *body);
  NodeType type() const override { return NodeType::LOOP; }
};

#endif // IR_H