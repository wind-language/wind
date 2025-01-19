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

DataType *typePriority(DataType *left, DataType *right) {
  // always prefer pointer types over scalar types
  if (left->isPointer()) {
    return left;
  } else if (right->isPointer()) {
    return right;
  }
  // if both are arrays, prefer the one with the highest capacity
  if (left->isArray() && right->isArray()) {
    if (left->getCaps() > right->getCaps()) {
      return left;
    } else {
      return right;
    }
  }
  // if one is an array and the other is not, prefer the array
  if (left->isArray()) {
    return left;
  } else if (right->isArray()) {
    return right;
  }
  // if both are scalars, prefer the one with the highest size
  if (left->moveSize() > right->moveSize()) {
    return left;
  } else {
    return right;
  }
  // if all else fails, return the left type
  return left;
}

DataType *findInferType(IRBinOp *binop) {
  switch (binop->operation()) {
    case IRBinOp::L_PLUS_ASSIGN:
    case IRBinOp::L_MINUS_ASSIGN:
    case IRBinOp::G_PLUS_ASSIGN:
    case IRBinOp::G_MINUS_ASSIGN:
    case IRBinOp::L_ASSIGN:
    case IRBinOp::G_ASSIGN:
    case IRBinOp::VA_ASSIGN:
      return binop->left()->inferType();
    default: {
      DataType *left = binop->left()->inferType();
      DataType *right = binop->right()->inferType();
      return typePriority(left, right);
    }
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
      op = IRBinOp::Operation::L_ASSIGN;
      right = new IRBinOp(
        std::unique_ptr<IRNode>(left),
        std::unique_ptr<IRNode>(right),
        IRBinOp::Operation::ADD
      );
    }
    else if (left->type() == IRNode::NodeType::GLOBAL_REF) {
      //op = IRBinOp::Operation::G_PLUS_ASSIGN;
      op = IRBinOp::Operation::G_ASSIGN;
      right = new IRBinOp(
        std::unique_ptr<IRNode>(left),
        std::unique_ptr<IRNode>(right),
        IRBinOp::Operation::ADD
      );
    }
    else {
      throw std::runtime_error("Left operand of += must be a variable reference (also not a pointer)");
    }
  }
  else if (node.getOperator() == "-=") {
    if (left->type() == IRNode::NodeType::LOCAL_REF) {
      // discontinued as unsafe (unchecked overflow)
      //op = IRBinOp::Operation::L_MINUS_ASSIGN;
      op = IRBinOp::Operation::L_ASSIGN;
      right = new IRBinOp(
        std::unique_ptr<IRNode>(left),
        std::unique_ptr<IRNode>(right),
        IRBinOp::Operation::SUB
      );
    } 
    else if (left->type() == IRNode::NodeType::GLOBAL_REF) {
      //op = IRBinOp::Operation::G_MINUS_ASSIGN;
      op = IRBinOp::Operation::G_ASSIGN;
      right = new IRBinOp(
        std::unique_ptr<IRNode>(left),
        std::unique_ptr<IRNode>(right),
        IRBinOp::Operation::SUB
      );
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
      op = IRBinOp::Operation::VA_ASSIGN;
    }
    else if (left->type() == IRNode::NodeType::GENERIC_INDEXING) {
      op = IRBinOp::Operation::GEN_INDEX_ASSIGN;
    }
    else {
      throw std::runtime_error("Left operand of assignment must be a variable reference or an indexed pointer");
    }
  } else { op = IRstr2op(node.getOperator()); }
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
  if (this->global_table.find(node.getName()) != this->global_table.end()) {
    return this->global_table[node.getName()];
  }
  else if (this->fn_table.find(node.getName()) != this->fn_table.end()) {
    return new IRFnRef(node.getName());
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
  if (!local->datatype()->isPointer() && !local->datatype()->isArray()) {
    throw std::runtime_error("Variable " + node.getName() + " is not a pointer or array");
  }
  this->current_fn->occupyOffset(local->offset());
  IRNode *index = nullptr;
  if (node.getIndex() != nullptr) {
    index = (IRNode*)node.getIndex()->accept(*this);
    if (index->type() == IRNode::NodeType::LITERAL) {
      uint16_t indexv = index->as<IRLiteral>()->get();
      if (local->datatype()->isArray() && local->datatype()->hasCapacity() && indexv >= local->datatype()->getCaps()) {
        throw std::runtime_error("Index " + std::to_string(indexv) + " out of bounds");
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
  const ASTNode *aval = node.get();
  if (aval == nullptr) {
    return new IRRet(nullptr);
  }
  IRNode *val = (IRNode*)aval->accept(*this);
  IRRet *ret = new IRRet(std::unique_ptr<IRNode>(val));
  return ret;
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
    if (this->decl_return) {
      this->decl_return = false;
      std::vector<IRVariableDecl*> *decls = (std::vector<IRVariableDecl*>*)child_node;
      for (IRVariableDecl *decl : *decls) {
        *body += std::unique_ptr<IRNode>(decl);
      }
    } else {
      if (child_node != nullptr) {
        if (child_node->type() == IRNode::NodeType::FUNCTION && child_node->as<IRFunction>()->isDefined) {
          body->addDefFn(child_node->as<IRFunction>()->fn_name);
        }
        *body += std::unique_ptr<IRNode>(child_node);
      }
    }
  }
  return body;
}

/**
 * @brief Visits a function node.
 * @param node The function node.
 * @return The compiled IR node.
 */
void* WindCompiler::visit(const Function &node) {
  IRFunction *fn = new IRFunction(node.getName(), {}, std::unique_ptr<IRBody>(new IRBody({})));
  fn->metadata = node.metadata;
  fn->isDefined = node.isDefined;
  auto found_fn = fn_table.find(node.getName());
  if (found_fn != this->fn_table.end() && found_fn->second->isDefined) {
    throw std::runtime_error("Function " + node.getName() + " already defined");
  }
  this->current_fn = fn;
  fn->return_type = this->ResolveDataType(node.getType());
  std::vector<DataType*> arg_types;
  for (std::string type : node.getArgTypes()) {
    arg_types.push_back(this->ResolveDataType(type));
  }
  fn->copyArgTypes(arg_types);
  this->fn_table[node.getName()] = fn;
  fn->flags = node.flags;
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

/**
 * @brief Visits a function call node.
 * @param node The function call node.
 * @return The compiled IR node.
 */
void* WindCompiler::visit(const FnCall &node) {
  this->current_fn->call_sub = true;

  auto fnit = this->fn_table.find(node.getName());
  if (fnit == this->fn_table.end()) {
    throw std::runtime_error("Function " + node.getName() + " not found");
  }

  IRFnCall *call = new IRFnCall(node.getName(), {}, fnit->second);
  for (const auto &arg : node.getArgs()) {
    call->push_arg(std::unique_ptr<IRNode>((IRNode*)arg->accept(*this)));
  }
  return call;
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
      while (i < code.size() && isalnum(code[i])) {
        var += code[i];
        i++;
      }
      i--; // compensate for the increment
      IRLocalRef *local = this->current_fn->GetLocal(var);
      if (local == nullptr) {
        throw std::runtime_error("[INLINE ASM] Variable " + var + " not found");
      }
      new_code += "[rbp-" + std::to_string(local->offset()) + "]";
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
  return new IRTryCatch(try_body, handlers);
}