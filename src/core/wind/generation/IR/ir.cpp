/**
 * @file ir.cpp
 * @brief Implementation of the IR classes for the Wind compiler.
 */

#include <wind/generation/IR.h>
#include <algorithm>
#include <stdexcept>

/**
 * @brief Constructor for IRRet.
 * @param v The return value.
 */
IRRet::IRRet(std::unique_ptr<IRNode> v) : value(std::move(v)) {}

/**
 * @brief Gets the return value.
 * @return The return value.
 */
IRNode* IRRet::get() const {
  return value.get();
}

/**
 * @brief Sets the return value.
 * @param v The return value.
 */
void IRRet::set(std::unique_ptr<IRNode> v) {
  value = std::move(v);
}

/**
 * @brief Constructor for IRLocalRef.
 * @param stack_offset The stack offset.
 * @param type The data type.
 */
IRLocalRef::IRLocalRef(uint16_t stack_offset, DataType *type) : stack_offset(stack_offset), var_type(type) {}

/**
 * @brief Gets the stack offset.
 * @return The stack offset.
 */
uint16_t IRLocalRef::offset() const {
  return stack_offset;
}

/**
 * @brief Gets the data type.
 * @return The data type.
 */
DataType* IRLocalRef::datatype() const {
  return var_type;
}

/**
 * @brief Constructor for IRLocalAddrRef.
 * @param stack_offset The stack offset.
 * @param type The data type.
 * @param index The index.
 */
IRLocalAddrRef::IRLocalAddrRef(uint16_t stack_offset, DataType* type, IRNode *index) : stack_offset(stack_offset), var_type(type), index(index) {}

/**
 * @brief Gets the stack offset.
 * @return The stack offset.
 */
uint16_t IRLocalAddrRef::offset() const {
  return stack_offset;
}

/**
 * @brief Gets the index.
 * @return The index.
 */
IRNode *IRLocalAddrRef::getIndex() const {
  return index;
}

/**
 * @brief Gets the data type.
 * @return The data type.
 */
DataType* IRLocalAddrRef::datatype() const {
  return var_type;
}

/**
 * @brief Constructor for IRBody.
 * @param s The statements in the body.
 */
IRBody::IRBody(std::vector<std::unique_ptr<IRNode>> s) : statements(std::move(s)) {}

/**
 * @brief Gets the statements in the body.
 * @return The statements.
 */
const std::vector<std::unique_ptr<IRNode>>& IRBody::get() const {
  return statements;
}

/**
 * @brief Sets a statement at the given index.
 * @param index The index.
 * @param statement The statement to set.
 */
void IRBody::Set(int index, std::unique_ptr<IRNode> statement) {
  statements[index] = std::move(statement);
}

/**
 * @brief Adds a statement to the body.
 * @param statement The statement to add.
 * @return The updated body.
 */
IRBody& IRBody::operator + (std::unique_ptr<IRNode> statement) {
  statements.push_back(std::move(statement));
  return *this;
}

/**
 * @brief Adds a statement to the body.
 * @param statement The statement to add.
 * @return The updated body.
 */
IRBody& IRBody::operator += (std::unique_ptr<IRNode> statement) {
  return *this + std::move(statement);
}

/**
 * @brief Constructor for IRFunction.
 * @param name The name of the function.
 * @param locals The local variables.
 * @param body The body of the function.
 */
IRFunction::IRFunction(std::string name, std::vector<std::unique_ptr<IRLocalRef>> locals, std::unique_ptr<IRBody> body) : fn_name(name), fn_locals(std::move(locals)), fn_body(std::move(body)) {}

/**
 * @brief Gets the name of the function.
 * @return The name of the function.
 */
const std::string& IRFunction::name() const {
  return fn_name;
}

/**
 * @brief Gets the local variables.
 * @return The local variables.
 */
const std::vector<std::unique_ptr<IRLocalRef>>& IRFunction::locals() const {
  return fn_locals;
}

/**
 * @brief Gets the body of the function.
 * @return The body of the function.
 */
IRBody* IRFunction::body() const {
  return fn_body.get();
}

/**
 * @brief Sets the body of the function.
 * @param body The body to set.
 */
void IRFunction::SetBody(std::unique_ptr<IRBody> body) {
  fn_body = std::move(body);
}

/**
 * @brief Checks if the function uses the stack.
 * @return True if the function uses the stack, false otherwise.
 */
bool IRFunction::isStack() {
  return stack_size > 0;
}

/**
 * @brief Occupies a stack offset.
 * @param offset The stack offset to occupy.
 */
void IRFunction::occupyOffset(uint16_t offset) {
  used_offsets.push_back(offset);
}

/**
 * @brief Checks if a local variable is used.
 * @param local The local variable.
 * @return True if the local variable is used, false otherwise.
 */
bool IRFunction::isUsed(IRLocalRef *local) {
  return std::find(used_offsets.begin(), used_offsets.end(), local->offset()) != used_offsets.end();
}

/**
 * @brief Copies the argument types.
 * @param types The argument types.
 */
void IRFunction::copyArgTypes(std::vector<DataType*> &types) {
  arg_types = types;
}

/**
 * @brief Gets the argument type at the given index.
 * @param index The index.
 * @return The argument type.
 */
DataType* IRFunction::GetArgType(int index) {
  return arg_types[index];
}

/**
 * @brief Clones the function.
 * @return The cloned function.
 */
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
  new_fn->ignore_stack_abi = ignore_stack_abi;
  new_fn->metadata = metadata;
  new_fn->canary_needed = canary_needed;
  return new_fn;
}

/**
 * @brief Creates a new local variable.
 * @param name The name of the variable.
 * @param type The data type.
 * @return The new local variable.
 */
IRLocalRef *IRFunction::NewLocal(std::string name, DataType *type) {
  uint16_t offset = stack_size+0x10; // left for canary
  offset += type->memSize();
  stack_size += type->memSize();
  local_table.insert({name, new IRLocalRef(offset, type)});
  this->fn_locals.push_back(std::make_unique<IRLocalRef>(offset, type));
  return this->fn_locals.back().get();
}

/**
 * @brief Gets a local variable by name.
 * @param name The name of the variable.
 * @return The local variable.
 */
IRLocalRef* IRFunction::GetLocal(std::string name) {
  auto it = local_table.find(name);
  if (it == local_table.end()) {
    return nullptr;
  }
  return it->second;
}

/**
 * @brief Constructor for IRBinOp.
 * @param l The left operand.
 * @param r The right operand.
 * @param o The operation.
 */
IRBinOp::IRBinOp(std::unique_ptr<IRNode> l, std::unique_ptr<IRNode> r, Operation o) : expr_left(std::move(l)), expr_right(std::move(r)), op(o) {}

/**
 * @brief Gets the left operand.
 * @return The left operand.
 */
const IRNode* IRBinOp::left() const {
  return expr_left.get();
}

/**
 * @brief Gets the right operand.
 * @return The right operand.
 */
const IRNode* IRBinOp::right() const {
  return expr_right.get();
}

/**
 * @brief Gets the operation.
 * @return The operation.
 */
IRBinOp::Operation IRBinOp::operation() const {
  return op;
}

/**
 * @brief Converts a string to an operation.
 * @param str The string representation of the operation.
 * @return The operation.
 */
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
  } else if (str == "==") {
    return IRBinOp::EQ;
  } else if (str == "<") {
    return IRBinOp::LESS;
  } else if (str == ">") {
    return IRBinOp::GREATER;
  } else if (str == "&&") {
    return IRBinOp::LOGAND;
  } else if (str == "%") {
    return IRBinOp::MOD;
  } else if (str == "<=") {
    return IRBinOp::LESSEQ;
  }
  else {
    throw std::runtime_error("Invalid operation");
  }
}

/**
 * @brief Constructor for IRLiteral.
 * @param v The literal value.
 */
IRLiteral::IRLiteral(long long v) : value(v) {}

/**
 * @brief Gets the literal value.
 * @return The literal value.
 */
long long IRLiteral::get() const {
  return value;
}

/**
 * @brief Constructor for IRStringLiteral.
 * @param v The string value.
 */
IRStringLiteral::IRStringLiteral(std::string v) : value(v) {}

/**
 * @brief Gets the string value.
 * @return The string value.
 */
const std::string& IRStringLiteral::get() const {
  return value;
}

/**
 * @brief Constructor for IRVariableDecl.
 * @param local_ref The local reference.
 * @param value The value.
 */
IRVariableDecl::IRVariableDecl(IRLocalRef* local_ref, IRNode* value) : local_ref(local_ref), v_value(value) {}

/**
 * @brief Gets the local reference.
 * @return The local reference.
 */
IRLocalRef* IRVariableDecl::local() const {
  return local_ref;
}

/**
 * @brief Gets the value.
 * @return The value.
 */
IRNode* IRVariableDecl::value() const {
  return this->v_value;
}

/**
 * @brief Constructor for IRGlobRef.
 * @param name The name of the global variable.
 * @param type The data type.
 */
IRGlobRef::IRGlobRef(std::string name, DataType *type) : name(name), g_type(type) {}

/**
 * @brief Gets the name of the global variable.
 * @return The name of the global variable.
 */
const std::string& IRGlobRef::getName() const {
  return name;
}

/**
 * @brief Gets the data type.
 * @return The data type.
 */
DataType* IRGlobRef::getType() const {
  return g_type;
}

/**
 * @brief Constructor for IRGlobalDecl.
 * @param glob_ref The global reference.
 * @param value The value.
 */
IRGlobalDecl::IRGlobalDecl(IRGlobRef* glob_ref, IRNode* value) : glob_ref(glob_ref), g_value(value) {}

/**
 * @brief Gets the global reference.
 * @return The global reference.
 */
IRGlobRef* IRGlobalDecl::global() const {
  return glob_ref;
}

/**
 * @brief Gets the value.
 * @return The value.
 */
IRNode* IRGlobalDecl::value() const {
  return g_value;
}

/**
 * @brief Constructor for IRArgDecl.
 * @param local_ref The local reference.
 */
IRArgDecl::IRArgDecl(IRLocalRef* local_ref) : local_ref(local_ref) {}

/**
 * @brief Gets the local reference.
 * @return The local reference.
 */
IRLocalRef* IRArgDecl::local() const {
  return local_ref;
}

/**
 * @brief Constructor for IRFnCall.
 * @param name The name of the function.
 * @param args The arguments.
 */
IRFnCall::IRFnCall(std::string name, std::vector<std::unique_ptr<IRNode>> args, IRFunction *ref) : fn_name(name), fn_args(std::move(args)), ref(ref) {}

/**
 * @brief Gets the name of the function.
 * @return The name of the function.
 */
const std::string& IRFnCall::name() const {
  return fn_name;
}

/**
 * @brief Gets the arguments.
 * @return The arguments.
 */
const std::vector<std::unique_ptr<IRNode>>& IRFnCall::args() const {
  return fn_args;
}

/**
 * @brief Gets the function reference.
 * @return The function reference.
 */
IRFunction *IRFnCall::getRef() const {
  return ref;
}

/**
 * @brief Replaces an argument at the given index.
 * @param index The index.
 * @param arg The argument to replace.
 */
void IRFnCall::replaceArg(int index, std::unique_ptr<IRNode> arg) {
  this->fn_args[index] = std::move(arg);
}

/**
 * @brief Pushes an argument to the function call.
 * @param arg The argument to push.
 */
void IRFnCall::push_arg(std::unique_ptr<IRNode> arg) {
  fn_args.push_back(std::move(arg));
}

/**
 * @brief Constructor for IRInlineAsm.
 * @param code The inline assembly code.
 */
IRInlineAsm::IRInlineAsm(std::string code) : asm_code(code) {}

/**
 * @brief Gets the inline assembly code.
 * @return The inline assembly code.
 */
const std::string &IRInlineAsm::code() const {
  return asm_code;
}

/**
 * @brief Constructor for IRBranching.
 * @param branches The branches.
 */
IRBranching::IRBranching(std::vector<IRBranch> &branches): branches(std::move(branches)) {}

/**
 * @brief Gets the branches.
 * @return The branches.
 */
const std::vector<IRBranch>& IRBranching::getBranches() const {
  return branches;
}

/**
 * @brief Sets the else branch.
 * @param body The else branch body.
 */
void IRBranching::setElseBranch(IRBody *body) {
  else_branch = body;
}

/**
 * @brief Gets the else branch.
 * @return The else branch.
 */
IRBody *IRBranching::getElseBranch() const {
  return else_branch;
}

/**
 * @brief Gets the condition of the loop.
 * @return The condition.
 */
IRNode *IRLooping::getCondition() const {
  return condition;
}

/**
 * @brief Gets the body of the loop.
 * @return The body.
 */
IRBody *IRLooping::getBody() const {
  return body;
}

/**
 * @brief Sets the condition of the loop.
 * @param c The condition.
 */
void IRLooping::setCondition(IRNode *c) {
  condition = c;
}

/**
 * @brief Sets the body of the loop.
 * @param b The body.
 */
void IRLooping::setBody(IRBody *b) {
  body = b;
}