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

/**
 * @brief Visits a binary expression node.
 * @param node The binary expression node.
 * @return The compiled IR node.
 */
void* WindCompiler::visit(const BinaryExpr &node) {
  IRNode *left = (IRNode*)node.getLeft()->accept(*this);
  IRNode *right = (IRNode*)node.getRight()->accept(*this);
  IRBinOp::Operation op = IRstr2op(node.getOperator());
  IRBinOp *binop = new IRBinOp(std::unique_ptr<IRNode>(left), std::unique_ptr<IRNode>(right), op);
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
  this->current_fn->occupyOffset(local->offset());
  return new IRLocalAddrRef(local->offset(), local->datatype(), node.getIndex());
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
    if (child_node != nullptr) {
      *body += std::unique_ptr<IRNode>(child_node);
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
  if (std::find(this->fn_names.begin(), this->fn_names.end(), node.getName()) != this->fn_names.end()) {
    throw std::runtime_error("Function " + node.getName() + " already defined");
  }
  this->fn_names.push_back(node.getName());
  fn->return_type = this->ResolveDataType(node.getType());
  std::vector<DataType*> arg_types;
  for (std::string type : node.getArgTypes()) {
    arg_types.push_back(this->ResolveDataType(type));
  }
  fn->copyArgTypes(arg_types);
  this->current_fn = fn;
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
DataType *WindCompiler::ResolveDataType(const std::string &type) {
  if (type == "byte") {
    return new DataType(DataType::Sizes::BYTE);
  }
  else if (type == "short") {
    return new DataType(DataType::Sizes::WORD);
  }
  else if (type == "int") {
    return new DataType(DataType::Sizes::DWORD);
  }
  else if (type == "long") {
    return new DataType(DataType::Sizes::QWORD);
  }
  else if (type == "void") {
    return new DataType(DataType::Sizes::VOID);
  }
  else if (type[0]=='[' && type[type.size()-1]==']') {
    uint32_t capacity = 0, size=0;
    std::string inner = type.substr(1, type.size()-2);
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
  else if (this->userdef_types_map.find(type) != this->userdef_types_map.end()) {
    return this->userdef_types_map[type];
  }

  throw std::runtime_error("Invalid type " +type);
}

/**
 * @brief Visits a variable declaration node.
 * @param node The variable declaration node.
 * @return The compiled IR node.
 */
void *WindCompiler::visit(const VariableDecl &node) {
  assert(this->current_fn != nullptr);
  if (node.getValue()) {
    IRNode *val = (IRNode*)node.getValue()->accept(*this);
    IRLocalRef *local = this->current_fn->NewLocal(node.getName(), this->ResolveDataType(node.getType()));
    return new IRVariableDecl(
      local,
      val
    );
  }
  return new IRVariableDecl(
    this->current_fn->NewLocal(node.getName(), this->ResolveDataType(node.getType())),
    nullptr
  );
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
  IRLocalRef *local = this->current_fn->NewLocal(node.getName(), this->ResolveDataType(node.getType()));
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
  std::regex localRefPattern(R"(\?\w+)");
  std::smatch match;
  while (std::regex_search(code, match, localRefPattern)) {
    std::string localRef = match.str().substr(1);
    IRLocalRef *local = this->current_fn->GetLocal(localRef);
    if (local) {
      std::string ref = wordstr(local->datatype()->moveSize()) + " ptr [rbp-" + std::to_string(local->offset()) + "]";
      code = std::regex_replace(code, localRefPattern, ref);
    } else {
      auto globit = this->global_table.find(localRef);
      if (globit == this->global_table.end()) {
        throw std::runtime_error("variable " + localRef + " not found");
      }
      std::string ref = wordstr(globit->second->getType()->moveSize()) + " ptr [" + globit->first + "]";
      code = std::regex_replace(code, localRefPattern, ref);
    }
  }
  IRInlineAsm *asm_node = new IRInlineAsm(code);
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