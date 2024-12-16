#include <wind/generation/IR.h>
#include <wind/bridge/ast.h>
#include <wind/generation/compiler.h>
#include <wind/common/debug.h>
#include <wind/processing/utils.h>
#include <algorithm>

#include <assert.h>

WindCompiler::WindCompiler(Body *program) : program(program), emission(new IRBody({})) {
  this->compile();
}
WindCompiler::~WindCompiler() {}

IRBody *WindCompiler::get() {
  return emission;
}

void WindCompiler::compile() {
  this->emission = (IRBody*)this->program->accept(*this);
}

void* WindCompiler::visit(const BinaryExpr &node) {
  IRNode *left = (IRNode*)node.getLeft()->accept(*this);
  IRNode *right = (IRNode*)node.getRight()->accept(*this);
  IRBinOp::Operation op = IRstr2op(node.getOperator());
  IRBinOp *binop = new IRBinOp(std::unique_ptr<IRNode>(left), std::unique_ptr<IRNode>(right), op);
  return binop;
}

void* WindCompiler::visit(const VariableRef &node) {
  assert(this->current_fn != nullptr);
  IRLocalRef *local = this->current_fn->GetLocal(node.getName());
  assert(local != nullptr);
  this->current_fn->occupyOffset(local->offset());
  return local;
}

void *WindCompiler::visit(const VarAddressing &node) {
  assert(this->current_fn != nullptr);
  IRLocalRef *local = this->current_fn->GetLocal(node.getName());
  assert(local != nullptr);
  this->current_fn->occupyOffset(local->offset());
  return new IRLocalAddrRef(local->offset(), local->datatype(), node.getIndex());
}

void* WindCompiler::visit(const Literal &node) {
  IRLiteral *lit = new IRLiteral(node.get());
  return lit;
}

void* WindCompiler::visit(const StringLiteral &node) {
  IRStringLiteral *str = new IRStringLiteral(node.getValue());
  return str;
}

void* WindCompiler::visit(const Return &node) {
  IRNode *val = (IRNode*)node.get()->accept(*this);
  IRRet *ret = new IRRet(std::unique_ptr<IRNode>(val));
  return ret;
}

void* WindCompiler::visit(const Body &node) {
  IRBody *body = new IRBody({});
  for (const auto &child : node.get()) {
    *body += (std::unique_ptr<IRNode>((IRNode*)child->accept(*this)));
  }
  return body;
}

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
  fn->flags = node.flags;
  IRBody *body = (IRBody*)node.getBody()->accept(*this);
  fn->SetBody(std::unique_ptr<IRBody>(body));
  return fn;
}

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

  throw std::runtime_error("Invalid type " +type);
}

void *WindCompiler::visit(const LocalDecl &node) {
  assert(this->current_fn != nullptr);
  if (node.getValue()) {
    IRNode *val = (IRNode*)node.getValue()->accept(*this);
    IRLocalRef *local = this->current_fn->NewLocal(node.getName(), this->ResolveDataType(node.getType()));
    return new IRLocalDecl(
      local,
      val
    );
  }
  return new IRLocalDecl(
    this->current_fn->NewLocal(node.getName(), this->ResolveDataType(node.getType())),
    nullptr
  );
}

void *WindCompiler::visit(const ArgDecl &node) {
  assert(this->current_fn != nullptr);
  IRLocalRef *local = this->current_fn->NewLocal(node.getName(), this->ResolveDataType(node.getType()));
  return new IRArgDecl(local);
}

void* WindCompiler::visit(const FnCall &node) {
  this->current_fn->call_sub = true;
  IRFnCall *call = new IRFnCall(node.getName(), {});
  for (const auto &arg : node.getArgs()) {
    call->push_arg(std::unique_ptr<IRNode>((IRNode*)arg->accept(*this)));
  }
  return call;
}

void *WindCompiler::visit(const InlineAsm &node) {
  IRInlineAsm *asm_node = new IRInlineAsm(node.getCode());
  return asm_node;
}