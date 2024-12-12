#include <wind/generation/IR.h>
#include <wind/bridge/ast.h>
#include <wind/generation/compiler.h>
#include <wind/common/debug.h>

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

void* WindCompiler::visit(const Literal &node) {
  IRLiteral *lit = new IRLiteral(node.get());
  return lit;
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
  std::vector<int> arg_types;
  for (std::string type : node.getArgTypes()) {
    arg_types.push_back(this->ResolveType(type));
  }
  fn->copyArgSizes(arg_types);
  this->current_fn = fn;
  fn->flags = node.flags;
  IRBody *body = (IRBody*)node.getBody()->accept(*this);
  fn->SetBody(std::unique_ptr<IRBody>(body));
  return fn;
}

uint16_t WindCompiler::ResolveType(const std::string &type) {
  if (type == "int") {
    return 4;
  } else if (type == "char") {
    return 1;
  } else {
    assert(false);
  }
}

void *WindCompiler::visit(const LocalDecl &node) {
  assert(this->current_fn != nullptr);
  if (node.getValue()) {
    IRNode *val = (IRNode*)node.getValue()->accept(*this);
    IRLocalRef *local = this->current_fn->NewLocal(node.getName(), this->ResolveType(node.getType()));
    return new IRLocalDecl(
      local,
      val
    );
  }
  return new IRLocalDecl(
    this->current_fn->NewLocal(node.getName(), this->ResolveType(node.getType())),
    nullptr
  );
}

void *WindCompiler::visit(const ArgDecl &node) {
  assert(this->current_fn != nullptr);
  IRLocalRef *local = this->current_fn->NewLocal(node.getName(), this->ResolveType(node.getType()));
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