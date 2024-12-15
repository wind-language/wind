#include <wind/generation/IR.h>
#include <algorithm>
#include <stdexcept>

IRRet::IRRet(std::unique_ptr<IRNode> v) : value(std::move(v)) {}
IRNode* IRRet::get() const {
  return value.get();
}
void IRRet::set(std::unique_ptr<IRNode> v) {
  value = std::move(v);
}

IRLocalRef::IRLocalRef(uint16_t stack_offset, DataType *type) : stack_offset(stack_offset), var_type(type) {}
uint16_t IRLocalRef::offset() const {
  return stack_offset;
}
DataType* IRLocalRef::datatype() const {
  return var_type;
}

IRLocalAddrRef::IRLocalAddrRef(uint16_t stack_offset) : stack_offset(stack_offset) {}
uint16_t IRLocalAddrRef::offset() const {
  return stack_offset;
}

IRBody::IRBody(std::vector<std::unique_ptr<IRNode>> s) : statements(std::move(s)) {}
const std::vector<std::unique_ptr<IRNode>>& IRBody::get() const {
  return statements;
}

void IRBody::Set(int index, std::unique_ptr<IRNode> statement) {
  statements[index] = std::move(statement);
}

IRBody& IRBody::operator + (std::unique_ptr<IRNode> statement) {
  statements.push_back(std::move(statement));
  return *this;
}
IRBody& IRBody::operator += (std::unique_ptr<IRNode> statement) {
  return *this + std::move(statement);
}

IRFunction::IRFunction(std::string name, std::vector<std::unique_ptr<IRLocalRef>> locals, std::unique_ptr<IRBody> body) : fn_name(name), fn_locals(std::move(locals)), fn_body(std::move(body)) {}
const std::string& IRFunction::name() const {
  return fn_name;
}
const std::vector<std::unique_ptr<IRLocalRef>>& IRFunction::locals() const {
  return fn_locals;
}
IRBody* IRFunction::body() const {
  return fn_body.get();
}
void IRFunction::SetBody(std::unique_ptr<IRBody> body) {
  fn_body = std::move(body);
}

bool IRFunction::isStack() {
  return stack_size > 0;
}

void IRFunction::occupyOffset(uint16_t offset) {
  used_offsets.push_back(offset);
}

bool IRFunction::isUsed(IRLocalRef *local) {
  return std::find(used_offsets.begin(), used_offsets.end(), local->offset()) != used_offsets.end();
}

void IRFunction::copyArgTypes(std::vector<DataType*> &types) {
  arg_types = types;
}

DataType* IRFunction::GetArgType(int index) {
  return arg_types[index];
}

IRFunction *IRFunction::clone() {
  IRFunction *new_fn = new IRFunction(fn_name, {}, std::unique_ptr<IRBody>(new IRBody({})));
  new_fn->stack_size = stack_size;
  new_fn->fn_locals = std::move(fn_locals);
  new_fn->local_table = std::move(local_table);
  new_fn->used_offsets = std::move(used_offsets);
  new_fn->flags = flags;
  new_fn->arg_types = arg_types;
  new_fn->call_sub = call_sub;
  new_fn->arg_types = arg_types;
  new_fn->return_type = return_type;
  return new_fn;
}

IRLocalRef *IRFunction::NewLocal(std::string name, DataType *type) {
  uint16_t offset = stack_size+0x08; // left for canary
  offset += type->memSize();
  stack_size += type->memSize();
  local_table.insert({name, new IRLocalRef(offset, type)});
  this->fn_locals.push_back(std::make_unique<IRLocalRef>(offset, type));
  return this->fn_locals.back().get();
}
IRLocalRef* IRFunction::GetLocal(std::string name) {
  auto it = local_table.find(name);
  if (it == local_table.end()) {
    return nullptr;
  }
  return it->second;
}

IRBinOp::IRBinOp(std::unique_ptr<IRNode> l, std::unique_ptr<IRNode> r, Operation o) : expr_left(std::move(l)), expr_right(std::move(r)), op(o) {}
const IRNode* IRBinOp::left() const {
  return expr_left.get();
}
const IRNode* IRBinOp::right() const {
  return expr_right.get();
}
IRBinOp::Operation IRBinOp::operation() const {
  return op;
}
IRBinOp::Operation IRstr2op(std::string str) {
  if (str == "+") {
    return IRBinOp::ADD;
  } else if (str == "-") {
    return IRBinOp::SUB;
  } else if (str == "*") {
    return IRBinOp::MUL;
  } else if (str == "/") {
    return IRBinOp::DIV;
  } else if (str == "<<") {
    return IRBinOp::SHL;
  } else if (str == ">>") {
    return IRBinOp::SHR;
  } else if (str == "=") {
    return IRBinOp::ASSIGN;
  }
  else {
    throw std::runtime_error("Invalid operation");
  }
}

IRLiteral::IRLiteral(long long v) : value(v) {}
long long IRLiteral::get() const {
  return value;
}

IRStringLiteral::IRStringLiteral(std::string v) : value(v) {}
const std::string& IRStringLiteral::get() const {
  return value;
}

IRLocalDecl::IRLocalDecl(IRLocalRef* local_ref, IRNode* value) : local_ref(local_ref), v_value(value) {}
IRLocalRef* IRLocalDecl::local() const {
  return local_ref;
}
IRNode* IRLocalDecl::value() const {
  return this->v_value;
}

IRArgDecl::IRArgDecl(IRLocalRef* local_ref) : local_ref(local_ref) {}
IRLocalRef* IRArgDecl::local() const {
  return local_ref;
}

IRFnCall::IRFnCall(std::string name, std::vector<std::unique_ptr<IRNode>> args) : fn_name(name), fn_args(std::move(args)) {}
const std::string& IRFnCall::name() const {
  return fn_name;
}
const std::vector<std::unique_ptr<IRNode>>& IRFnCall::args() const {
  return fn_args;
}
void IRFnCall::push_arg(std::unique_ptr<IRNode> arg) {
  fn_args.push_back(std::move(arg));
}

IRRegister::IRRegister(asmjit::x86::Gp reg) : v_reg(reg) {}
asmjit::x86::Gp IRRegister::reg() const {
  return v_reg;
}

IRInlineAsm::IRInlineAsm(std::string code) : asm_code(code) {}
const std::string &IRInlineAsm::code() const {
  return asm_code;
}