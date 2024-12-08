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
    LOCAL_DECL,
    ARG_DECL,
    FUNCTION_CALL,
    REGISTER
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
  uint16_t var_size;

public:
  IRLocalRef(uint16_t stack_offset, uint16_t size);
  uint16_t offset() const;
  uint16_t size() const;
  NodeType type() const override { return NodeType::LOCAL_REF; }
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
  uint16_t stack_size = 4;
  std::vector<uint16_t> used_offsets;
  FnFlags flags = 0;
  std::vector<int> arg_sizes;

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
  void copyArgSizes(std::vector<int> &types);
  int GetArgSize(int index);

  IRLocalRef *NewLocal(std::string name, uint16_t size);
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
    SHR
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

class IRLocalDecl : public IRNode {
  IRLocalRef *local_ref;
  IRNode *v_value;

public:
  IRLocalDecl(IRLocalRef* local_ref, IRNode* value);
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
  NodeType type() const override { return NodeType::FUNCTION_CALL; }
};

class IRRegister : public IRNode {
  asmjit::x86::Gp v_reg;

public:
  explicit IRRegister(asmjit::x86::Gp r);
  asmjit::x86::Gp reg() const;
  NodeType type() const override { return NodeType::REGISTER; }
};

#endif // IR_H