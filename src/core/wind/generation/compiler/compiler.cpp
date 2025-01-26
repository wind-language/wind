/**
 * @file compiler.cpp
 * @brief Implementation of the WindCompiler class.
 */

#include <wind/generation/IR.h>
#include <wind/bridge/ast.h>
#include <wind/generation/compiler.h>
#include <wind/common/debug.h>
#include <wind/processing/utils.h>
#include <algorithm>
#include <regex>

#include <assert.h>

/**
 * @brief Constructor for WindCompiler.
 * @param program The AST body of the program.
 */
WindCompiler::WindCompiler(Body *program) : program(program), emission(new IRBody({})) {
  this->compile();
}

/**
 * @brief Destructor for WindCompiler.
 */
WindCompiler::~WindCompiler() {}

/**
 * @brief Gets the IR body after compilation.
 * @return The compiled IR body.
 */
IRBody *WindCompiler::get() {
  return emission;
}

/**
 * @brief Compiles the AST into IR.
 */
void WindCompiler::compile() {
  this->emission = (IRBody*)this->program->accept(*this);
}

DataType *typePriority(const IRNode *left, const IRNode *right) {
  DataType *left_t = left->inferType();
  DataType *right_t = right->inferType();
  // always prefer pointer types over scalar types
  if (left_t->isPointer()) {
    return left_t;
  } else if (right_t->isPointer()) {
    return right_t;
  }
  // if both are arrays, prefer the one with the highest capacity
  if (left_t->isArray() && right_t->isArray()) {
    if (left_t->getCaps() > right_t->getCaps()) {
      return left_t;
    } else {
      return right_t;
    }
  }
  // if one is an array and the other is not, prefer the array
  if (left_t->isArray()) {
    return left_t;
  } else if (right_t->isArray()) {
    return right_t;
  }
  // if both are scalars, prefer the one with the highest size
  if (left->is<IRLiteral>()) return right_t;
  if (right->is<IRLiteral>()) return left_t;
  if (left_t->moveSize() > right_t->moveSize()) {
    return left_t;
  } else {
    return right_t;
  }
  // if all else fails, return the left_t type
  return left_t;
}

DataType *findInferType(IRBinOp *binop) {
  switch (binop->operation()) {
    case IRBinOp::L_ASSIGN:
    case IRBinOp::G_ASSIGN:
    case IRBinOp::GEN_INDEX_ASSIGN:
      return binop->left()->inferType();
    default: {
      return typePriority(binop->left(), binop->right());
    }
  }
}

void WindCompiler::CanCoerce(IRNode *left, IRNode *right) {
  DataType *left_t = left->inferType();
  DataType *right_t = right->inferType();
  if (left->is<IRLiteral>() || right->is<IRLiteral>()) return;
  if (left_t->isVoid() || right_t->isVoid()) {
    throw std::runtime_error("Cannot coerce void type");
  }
  if (left_t->isPointer() || right_t->isPointer()) return;
  if (left_t->isArray() || right_t->isArray()) return;
  if (left_t->moveSize() != right_t->moveSize()) {
    throw std::runtime_error("Cannot coerce types of different sizes, cast required");
  }
}

/**
 * @brief Visits a binary expression node.
 * @param node The binary expression node.
 * @return The compiled IR node.
 */
void* WindCompiler::visit(const BinaryExpr &node) {
  IRNode *left = (IRNode*)node.getLeft()->accept(*this);
  IRNode *right = (IRNode*)node.getRight()->accept(*this);
  IRBinOp::Operation op;
  if (node.getOperator() == "+=") {
    if (left->type() == IRNode::NodeType::LOCAL_REF) {
      // discontinued as unsafe (unchecked overflow)
      // op = IRBinOp::Operation::L_PLUS_ASSIGN;
      return (IRNode*)(new BinaryExpr(
        std::unique_ptr<ASTNode>((ASTNode*)node.getLeft()),
        std::unique_ptr<ASTNode>(new BinaryExpr(
          std::unique_ptr<ASTNode>((ASTNode*)node.getLeft()),
          std::unique_ptr<ASTNode>((ASTNode*)node.getRight()),
          "+"
        )),
        "="
      ))->accept(*this);
    }
    else if (left->type() == IRNode::NodeType::GLOBAL_REF) {
      //op = IRBinOp::Operation::G_PLUS_ASSIGN;
      return (IRNode*)(new BinaryExpr(
        std::unique_ptr<ASTNode>((ASTNode*)node.getLeft()),
        std::unique_ptr<ASTNode>(new BinaryExpr(
          std::unique_ptr<ASTNode>((ASTNode*)node.getLeft()),
          std::unique_ptr<ASTNode>((ASTNode*)node.getRight()),
          "+"
        )),
        "="
      ))->accept(*this);
    }
    else {
      throw std::runtime_error("Left operand of += must be a variable reference (also not a pointer)");
    }
  }
  else if (node.getOperator() == "-=") {
    if (left->type() == IRNode::NodeType::LOCAL_REF) {
      // discontinued as unsafe (unchecked overflow)
      //op = IRBinOp::Operation::L_MINUS_ASSIGN;
      return (IRNode*)(new BinaryExpr(
        std::unique_ptr<ASTNode>((ASTNode*)node.getLeft()),
        std::unique_ptr<ASTNode>(new BinaryExpr(
          std::unique_ptr<ASTNode>((ASTNode*)node.getLeft()),
          std::unique_ptr<ASTNode>((ASTNode*)node.getRight()),
          "-"
        )),
        "="
      ))->accept(*this);
    } 
    else if (left->type() == IRNode::NodeType::GLOBAL_REF) {
      //op = IRBinOp::Operation::G_MINUS_ASSIGN;
      return (IRNode*)(new BinaryExpr(
        std::unique_ptr<ASTNode>((ASTNode*)node.getLeft()),
        std::unique_ptr<ASTNode>(new BinaryExpr(
          std::unique_ptr<ASTNode>((ASTNode*)node.getLeft()),
          std::unique_ptr<ASTNode>((ASTNode*)node.getRight()),
          "-"
        )),
        "="
      ))->accept(*this);
    }
    else {
      throw std::runtime_error("Left operand of -= must be a variable reference (also not a pointer)");
    }
  }
  else if (node.getOperator() == "=") {
    // Assign operation
    if (left->type() == IRNode::NodeType::LOCAL_REF) {
      op = IRBinOp::Operation::L_ASSIGN;
    } 
    else if (left->type() == IRNode::NodeType::GLOBAL_REF) {
      op = IRBinOp::Operation::G_ASSIGN;
    }
    else if (left->type() == IRNode::NodeType::LADDR_REF) {
      op = IRBinOp::Operation::GEN_INDEX_ASSIGN;
    }
    else if (left->type() == IRNode::NodeType::GENERIC_INDEXING) {
      op = IRBinOp::Operation::GEN_INDEX_ASSIGN;
    }
    else {
      throw std::runtime_error("Left operand of assignment must be a variable reference or an indexed pointer");
    }
  } else {
    op = IRstr2op(node.getOperator());
    this->CanCoerce(left, right);
  }
  IRBinOp *binop = new IRBinOp(std::unique_ptr<IRNode>(left), std::unique_ptr<IRNode>(right), op);
  binop->setInferedType(findInferType(binop));
  return binop;
}

/**
 * @brief Visits a variable reference node.
 * @param node The variable reference node.
 * @return The compiled IR node.
 */
void* WindCompiler::visit(const VariableRef &node) {
  assert(this->current_fn != nullptr);
  IRLocalRef *local = this->current_fn->GetLocal(node.getName());
  if (local != nullptr) {
    this->current_fn->occupyOffset(local->offset());
    return local;
  }

  auto fn_it = this->fn_table.find(node.getName()); // find first to avoid ambiguity
  if (this->global_table.find(node.getName()) != this->global_table.end()) {
    return this->global_table[node.getName()];
  }
  else if (fn_it != this->fn_table.end()) {
    return new IRFnRef(fn_it->second->name());
  }
  throw std::runtime_error("Variable " + node.getName() + " not found");
}

/**
 * @brief Visits a variable addressing node.
 * @param node The variable addressing node.
 * @return The compiled IR node.
 */
void *WindCompiler::visit(const VarAddressing &node) {
  assert(this->current_fn != nullptr);
  IRLocalRef *local = this->current_fn->GetLocal(node.getName());
  if (local == nullptr) {
    throw std::runtime_error("Variable " + node.getName() + " not found");
  }
  if (node.getIndex() && !local->datatype()->isPointer() && !local->datatype()->isArray()) {
    throw std::runtime_error("Variable " + node.getName() + " is not a pointer or array");
  }
  this->current_fn->occupyOffset(local->offset());
  IRNode *index = nullptr;
  if (node.getIndex() != nullptr) {
    index = (IRNode*)node.getIndex()->accept(*this);
    if (local->datatype()->isArray() && index->type() == IRNode::NodeType::LITERAL) {
      uint16_t indexv = index->as<IRLiteral>()->get();
      if (local->datatype()->isArray() && local->datatype()->hasCapacity() && indexv >= local->datatype()->getCaps()) {
        throw std::runtime_error("Index " + std::to_string(indexv) + " out of bounds");
      }
    } else {
      if (!local->datatype()->isArray()) {
        // turn it into generic indexing
        return new IRGenericIndexing(local, index);
      }
    }
  }
  return new IRLocalAddrRef(local->offset(), local->datatype(), index);
}

/**
 * @brief Visits a literal node.
 * @param node The literal node.
 * @return The compiled IR node.
 */
void* WindCompiler::visit(const Literal &node) {
  IRLiteral *lit = new IRLiteral(node.get());
  return lit;
}

/**
 * @brief Visits a string literal node.
 * @param node The string literal node.
 * @return The compiled IR node.
 */
void* WindCompiler::visit(const StringLiteral &node) {
  IRStringLiteral *str = new IRStringLiteral(node.getValue());
  return str;
}

/**
 * @brief Visits a return statement node.
 * @param node The return statement node.
 * @return The compiled IR node.
 */
void* WindCompiler::visit(const Return &node) {
  if (this->current_fn->return_type->moveSize() == DataType::VOID) {
    throw std::runtime_error("Cannot return a value from a void function");
  }
  const ASTNode *aval = node.get();
  if (aval == nullptr) {
    return new IRRet(nullptr);
  }
  IRNode *val = (IRNode*)aval->accept(*this);
  IRRet *ret = new IRRet(std::unique_ptr<IRNode>(val));
  return ret;
}

void WindCompiler::NodeIntoBody(IRNode *node, IRBody *body) {
  if (node == nullptr) return;
  if (this->decl_return) {
    this->decl_return = false;
    std::vector<IRVariableDecl*> *decls = (std::vector<IRVariableDecl*>*)node;
    for (IRVariableDecl *decl : *decls) {
      *body += std::unique_ptr<IRNode>(decl);
    }
    return;
  }

  switch (node->type()) {
    case IRNode::NodeType::BODY: {
      IRBody *b = node->as<IRBody>();
      if (b == nullptr) {
        throw std::runtime_error("Invalid body node");
      }
      for (auto &sub_node : b->get()) {
        NodeIntoBody(sub_node.get(), body);
      }
      break;
    }
    case IRNode::NodeType::FUNCTION: {
      IRFunction *fn = node->as<IRFunction>();
      if (fn->isDefined) {
        body->addDefFn(fn->fn_name);
      }
    }
    default: {
      *body += std::unique_ptr<IRNode>(node);
    }
  }
}

/**
 * @brief Visits a body node.
 * @param node The body node.
 * @return The compiled IR node.
 */
void* WindCompiler::visit(const Body &node) {
  IRBody *body = new IRBody({});
  for (const auto &child : node.get()) {
    auto *child_node = (IRNode*)child->accept(*this);
    NodeIntoBody(child_node, body);
  }
  return body;
}

std::string WindCompiler::namespacePrefix(std::string name) {
  std::string prefix = "";
  for (std::string ns : this->active_namespaces) {
    prefix += ns + "::";
  }
  return prefix + name;
}

std::string WindCompiler::generalizeType(DataType *type) {
  if (type->isPointer()) {
    return generalizeType(type->getPtrType()) + "*";
  }
  else if (type->isArray()) {
    return "[" + std::to_string(type->getCaps()) + ";" + generalizeType(type->getArrayType()) + "]";
  }
  return (type->isSigned() ? "i" : "u")+std::to_string(type->moveSize()*8); // get u/s bitness
}

#include <wind/hashing/xxhash.h>
std::string WindCompiler::functionSignature(std::string name, DataType *ret, std::vector<DataType*> &args) {
  std::string sig = name+"(";
  for (DataType *arg : args) {
    sig += generalizeType(arg) + ",";
  }
  sig.pop_back();
  sig += ")";
  sig += "->"+generalizeType(ret);
  return sig;
}

std::string hashIntoHex(uint64_t hash) {
  std::string hex = "";
  for (int i=0; i<8; i++) {
    uint8_t byte = (hash >> (i*8)) & 0xFF;
    hex = hex + "0123456789abcdef"[byte >> 4] + "0123456789abcdef"[byte & 0x0F];
  }
  return hex;
}

std::pair<std::string, std::string> WindCompiler::fnSignHash(std::string name, DataType *ret, std::vector<DataType*> &args) {
  std::string plain_sign = functionSignature(name, ret, args);
  uint64_t long_hash = XXHash64::hash(plain_sign.c_str(), plain_sign.size());
  return {plain_sign, hashIntoHex(long_hash)};
}

#define PREFIX_FN(hash) "func_"+hash

void *WindCompiler::visit(const Function &node) {
  std::string raw_name = node.getName();
  std::string nsd_name = namespacePrefix(raw_name);

  IRFunction *fn = new IRFunction(nsd_name, {}, std::unique_ptr<IRBody>(new IRBody({})));
  fn->isDefined = node.isDefined;
  fn->flags = node.flags;
  if (nsd_name == "main") {
    fn->flags |= FN_PUBLIC | FN_NOMANGLE; // main function is always public and not mangled
  }
  this->current_fn = fn;

  std::vector<DataType*> arg_types;
  for (std::string type : node.getArgTypes()) {
    arg_types.push_back(this->ResolveDataType(type));
  }
  DataType *ret_type = this->ResolveDataType(node.getType());
  
  auto sign_hash = this->fnSignHash(nsd_name, ret_type, arg_types);
  fn->metadata = sign_hash.first + " [" + node.metadata+ "]";

  fn->return_type = ret_type;
  fn->copyArgTypes(arg_types);

  if (!(fn->flags & FN_NOMANGLE)) {
    fn->fn_name = PREFIX_FN(sign_hash.second);
  }

  this->fn_table.insert({nsd_name, fn});
  
  IRBody *body = (IRBody*)node.getBody()->accept(*this);
  fn->SetBody(std::unique_ptr<IRBody>(body));
  this->current_fn = nullptr;

  return fn;
}

/**
 * @brief Resolves a data type from its string representation.
 * @param type The string representation of the data type.
 * @return The resolved data type.
 */
DataType *WindCompiler::ResolveDataType(const std::string &n_type) {
  if (n_type[0]=='[' && n_type[n_type.size()-1]==']') {
    uint32_t capacity = 0, size=0;
    std::string inner = n_type.substr(1, n_type.size()-2);
    // if semicolon is present, there's capacity
    if (inner.find(';') != std::string::npos) {
      std::string cap = inner.substr(inner.find(';')+1);
      inner = inner.substr(0, inner.find(';'));
      capacity = fmtinttostr(cap);
    }
    DataType *intype = this->ResolveDataType(inner);
    size = intype->moveSize();
    if (capacity != 0) {
      return new DataType(size, capacity, intype);
    }
    return new DataType(size, intype);
  }
  else if (n_type.substr(0, 4) == "ptr<") {
    std::string inner = n_type.substr(4, n_type.size()-5);
    DataType *intype = this->ResolveDataType(inner);
    return new DataType(intype);
  }
  bool unsigned_type = n_type.find("unsigned") != std::string::npos;
  size_t sf = n_type.find(" ");
  std::string type = n_type.substr((sf!=std::string::npos) ? sf+1 : 0);
  type.erase(std::remove(type.begin(), type.end(), ' '), type.end());
  if (type == "byte") {
    return new DataType(DataType::Sizes::BYTE, !unsigned_type);
  }
  else if (type == "short") {
    return new DataType(DataType::Sizes::WORD, !unsigned_type);
  }
  else if (type == "int") {
    return new DataType(DataType::Sizes::DWORD, !unsigned_type);
  }
  else if (type == "long") {
    return new DataType(DataType::Sizes::QWORD, !unsigned_type);
  }
  else if (type == "void") {
    return new DataType(DataType::Sizes::VOID, false);
  }
  else if (this->userdef_types_map.find(type) != this->userdef_types_map.end()) {
    return this->userdef_types_map[type];
  }
  std::string fn_name;
  if (this->current_fn) {
    fn_name = " in function: " + this->current_fn->name();
  }
  throw std::runtime_error("Invalid type " +type + fn_name);
}

/**
 * @brief Visits a variable declaration node.
 * @param node The variable declaration node.
 * @return The compiled IR node.
 */
void *WindCompiler::visit(const VariableDecl &node) {
  assert(this->current_fn != nullptr);
  IRNode *val=nullptr;
  if (node.getValue()) {
    val = (IRNode*)node.getValue()->accept(*this);
  }
  std::vector<IRVariableDecl*> *decls = new std::vector<IRVariableDecl*>();
  for (std::string name : node.getNames()) {
    uint16_t decl_i = this->current_fn->locals().size();
    IRLocalRef *local = this->current_fn->NewLocal(name, this->ResolveDataType(node.getType()));
    if (local->datatype()->isArray()) {
      this->current_fn->canary_needed = true;
    }
    decls->push_back(new IRVariableDecl(local, val));
  }
  this->decl_return = true;
  return decls;
}

/**
 * @brief Visits a global declaration node.
 * @param node The global declaration node.
 * @return The compiled IR node.
 */
void *WindCompiler::visit(const GlobalDecl &node) {
  IRNode *val = nullptr;
  if (node.getValue()) {
    val = (IRNode*)node.getValue()->accept(*this);
    if (!val->is<IRLiteral>() && !val->is<IRStringLiteral>()) {
      throw std::runtime_error("Global declaration must have a literal value");
    }
  }
  IRGlobRef *glob = new IRGlobRef(node.getName(), this->ResolveDataType(node.getType()));
  this->global_table[node.getName()] = glob;
  return new IRGlobalDecl(glob, val);
}

/**
 * @brief Visits an argument declaration node.
 * @param node The argument declaration node.
 * @return The compiled IR node.
 */
void *WindCompiler::visit(const ArgDecl &node) {
  assert(this->current_fn != nullptr);
  IRLocalRef *local = this->current_fn->NewLocal(node.getName(), this->ResolveDataType(node.getType()), this->current_fn->locals().size() >= 6);
  return new IRArgDecl(local);
}

bool WindCompiler::typeMatch(DataType *left, IRNode *right) {
  DataType *right_t = right->inferType();
  if (left->isPointer() && right_t->isPointer()) return true;
  if (left->isArray() && right_t->isArray()) return true;
  if (left->isScalar() && right_t->isScalar()) {
    if (right->is<IRLiteral>()) return true;
    if (left->moveSize() == right_t->moveSize()) return true;
  }
  return false;
}

IRFunction *WindCompiler::matchFunction(std::string name, std::vector<std::unique_ptr<IRNode>> &args, bool raise) {
  auto range = this->fn_table.equal_range(name);
  if (range.first == range.second) {
    if (!raise) return nullptr;
    throw std::runtime_error("Function " + name + " not found");
  }
  for (auto it = range.first; it != range.second; it++) {
    IRFunction *fn = it->second;
    if ((fn->arg_types.size() != args.size()) && !(fn->flags & FN_VARIADIC) && !(fn->flags & FN_ARGPUSH)) continue;
    bool match = true;
    for (int i=0; i<fn->arg_types.size();i++) {
      if (!this->typeMatch(fn->arg_types[i], args[i].get())) {
        match = false;
        break;
      }
    }
    if (match) {
      return fn;
    }
  }
  if (raise)
    throw std::runtime_error("No " + name + " function overload matches the arguments");
  return nullptr;
}

/**
 * @brief Visits a function call node.
 * @param node The function call node.
 * @return The compiled IR node.
 */
void* WindCompiler::visit(const FnCall &node) {
  this->current_fn->call_sub = true;

  std::vector<std::unique_ptr<IRNode>> args;
  for (const auto &arg : node.getArgs()) {
    args.push_back(std::unique_ptr<IRNode>((IRNode*)arg->accept(*this)));
  }

  IRFunction* fn = this->matchFunction(node.getName(), args);
  return new IRFnCall(fn->name(), std::move(args), fn);
}


std::string wordstr(int size) {
  switch (size) {
    case 1:
      return "byte";
    case 2:
      return "word";
    case 4:
      return "dword";
    case 8:
      return "qword";
    default:
      return "qword";
  }
}

/**
 * @brief Visits an inline assembly node.
 * @param node The inline assembly node.
 * @return The compiled IR node.
 */
void *WindCompiler::visit(const InlineAsm &node) {
  std::string code = node.getCode();
  std::string new_code = "";
  
  for (int i=0;i<code.size();i++) {
    if (code[i] == '?') {
      std::string var = "";
      i++;
      while (i < code.size() && LexUtils::alphanum(code[i])) {
        var += code[i];
        i++;
      }
      i--; // compensate for the increment
      IRLocalRef *local = this->current_fn->GetLocal(var);
      if (local == nullptr) {
        throw std::runtime_error("[INLINE ASM] Local variable " + var + " not found");
      }
      new_code += "[rbp" + std::to_string(local->offset()) + "]";
    } else {
      new_code += code[i];
    }
  }

  IRInlineAsm *asm_node = new IRInlineAsm(new_code);
  return asm_node;
}

/**
 * @brief Visits a type declaration node.
 * @param node The type declaration node.
 * @return The compiled IR node.
 */
void *WindCompiler::visit(const TypeDecl &node) {
  DataType *type = this->ResolveDataType(node.getType());
  this->userdef_types_map[node.getName()] = type;
  return nullptr;
}

/**
 * @brief Visits a branching node.
 * @param node The branching node.
 * @return The compiled IR node.
 */
void *WindCompiler::visit(const Branching &node) {
  std::vector<IRBranch> branches;
  IRBody *else_branch = nullptr;

  for (const auto &branch : node.getBranches()) {
    IRNode *cond = (IRNode*)branch.condition->accept(*this);
    IRBody *body = (IRBody*)branch.body->accept(*this);
    branches.push_back({std::unique_ptr<IRNode>(cond), std::unique_ptr<IRNode>(body)});
  }
  if (node.getElseBranch()) {
    else_branch = (IRBody*)node.getElseBranch()->accept(*this);
  }

  IRBranching *brir = new IRBranching(branches);
  brir->setElseBranch(else_branch);
  return brir;
}

/**
 * @brief Visits a looping node.
 * @param node The looping node.
 * @return The compiled IR node.
 */
void *WindCompiler::visit(const Looping &node) {
  IRNode *cond = (IRNode*)node.getCondition()->accept(*this);
  IRBody *body = (IRBody*)node.getBody()->accept(*this);
  IRLooping *loop = new IRLooping();
  loop->setCondition(cond);
  loop->setBody(body);
  return loop;
}

/**
 * @brief Visits a break node.
 * @param node The break node.
 * @return The compiled IR node.
 */
void *WindCompiler::visit(const Break &node) {
  return new IRBreak();
}

/**
 * @brief Visits a continue node.
 * @param node The continue node.
 * @return The compiled IR node.
 */
void *WindCompiler::visit(const Continue &node) {
  return new IRContinue();
}

void *WindCompiler::visit(const GenericIndexing &node) {
  IRNode *base = (IRNode*)node.getBase()->accept(*this);
  IRNode *index = (IRNode*)node.getIndex()->accept(*this);
  if (!base->inferType()->isPointer()) {
    throw std::runtime_error("Base of indexing must be a pointer");
  }
  return new IRGenericIndexing(base, index);
}

void *WindCompiler::visit(const PtrGuard &node) {
  IRNode *value = (IRNode*)node.getValue()->accept(*this);
  return new IRPtrGuard(value);
}

void *WindCompiler::visit(const TypeCast &node) {
  IRNode *value = (IRNode*)node.getValue()->accept(*this);
  DataType *type = this->ResolveDataType(node.getType());
  return new IRTypeCast(value, type);
}

void *WindCompiler::visit(const SizeOf &node) {
  DataType *type = this->ResolveDataType(node.getType());
  return new IRLiteral(type->moveSize());
}

void *WindCompiler::visit(const TryCatch &node) {
  IRBody *try_body = (IRBody*)node.getTryBody()->accept(*this);
  std::map<HandlerType, IRBody*> handlers;
  for (const auto &handler : node.getCatchBlocks()) {
    auto matched_handler = handler_map.find(handler.first);
    if (matched_handler == handler_map.end()) {
      throw std::runtime_error("Invalid handler type " + handler.first);
    }
    handlers[matched_handler->second] = (IRBody*)handler.second->accept(*this);
  }
  IRBody *finally_body = nullptr;
  if (node.getFinallyBlock()) {
    finally_body = (IRBody*)node.getFinallyBlock()->accept(*this);
  }
  return new IRTryCatch(try_body, finally_body, handlers);
}

void *WindCompiler::visit(const Namespace &node) {
  this->active_namespaces.push_back(node.getName());
  Body *body = node.getChildren();
  if (body == nullptr) {
    throw std::runtime_error("Namespace " + node.getName() + " has no body");
  }
  IRBody *irbody = (IRBody*)body->accept(*this);
  this->active_namespaces.pop_back();
  return irbody;
}