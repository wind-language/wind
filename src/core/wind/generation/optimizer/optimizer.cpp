#include <wind/generation/IR.h>
#include <wind/generation/optimizer.h>
#include <wind/bridge/flags.h>
#include <wind/common/debug.h>

#include <unordered_set>
#include <cstring>
#include <cassert>
#include <iostream>

const std::unordered_set<IRBinOp::Operation> UselessZeroTable = {
  IRBinOp::Operation::ADD,
  IRBinOp::Operation::SUB,
  IRBinOp::Operation::SHL,
};

const std::unordered_set<IRBinOp::Operation> NoOrderTable = {
  IRBinOp::Operation::ADD,
  IRBinOp::Operation::MUL,
  IRBinOp::Operation::EQ,
  IRBinOp::Operation::AND
};

WindOptimizer::WindOptimizer(IRBody *program) : program(program), emission(new IRBody({})) {
  optimize();
}
WindOptimizer::~WindOptimizer() {
  return;
}

IRLiteral *WindOptimizer::OptimizeConstFold(IRBinOp *node) {
  long long left = node->left()->as<IRLiteral>()->get();
  long long right = node->right()->as<IRLiteral>()->get();
  IRBinOp::Operation op = node->operation();

  switch (op) {
    case IRBinOp::Operation::ADD:
      return new IRLiteral(left + right);
    case IRBinOp::Operation::SUB:
      return new IRLiteral(left - right);
    case IRBinOp::Operation::MUL:
      return new IRLiteral(left * right);
    case IRBinOp::Operation::DIV:
      return new IRLiteral(left / right);
    case IRBinOp::Operation::SHL:
      return new IRLiteral(left << right);
    case IRBinOp::Operation::SHR:
      return new IRLiteral(left >> right);
    case IRBinOp::Operation::AND:
      return new IRLiteral(left & right);
    case IRBinOp::Operation::EQ:
      return new IRLiteral(left == right);
    case IRBinOp::Operation::LESS:
      return new IRLiteral(left < right);
    case IRBinOp::Operation::GREATER:
      return new IRLiteral(left > right);
    case IRBinOp::Operation::LESSEQ:
      return new IRLiteral(left <= right);
    case IRBinOp::Operation::L_ASSIGN:
    case IRBinOp::Operation::G_ASSIGN:
      return new IRLiteral(right);
    case IRBinOp::Operation::MOD:
      return new IRLiteral(left % right);
    case IRBinOp::Operation::OR:
      return new IRLiteral(left | right);
    case IRBinOp::Operation::XOR:
      return new IRLiteral(left ^ right);
    case IRBinOp::Operation::NOTEQ:
      return new IRLiteral(left != right);
    case IRBinOp::Operation::GREATEREQ:
      return new IRLiteral(left >= right);
    default:
      return nullptr;
  }
}

bool pow2(long long n) {
  return (n & (n - 1)) == 0;
}


IRNode *WindOptimizer::OptimizeBinOp(IRBinOp *node, bool canLocFold) {
  IRNode *left = (IRNode*)node->left();
  IRNode *right = (IRNode*)node->right();
  IRBinOp::Operation op = node->operation();
  if (this->current_fn->fn->flags & PURE_EXPR) {
    return node;
  }
  IRNode *opt_left = left;

  if (op != IRBinOp::L_ASSIGN) {
    opt_left = this->OptimizeExpr(left, canLocFold);
  }
  IRNode *opt_right = this->OptimizeExpr(right, canLocFold);

  if (opt_left->is<IRLiteral>() && opt_right->is<IRLiteral>()) {
    IRBinOp *binop = new IRBinOp(std::unique_ptr<IRNode>(opt_left), std::unique_ptr<IRNode>(opt_right), op);
    binop->setInferedType(node->inferType());
    return this->OptimizeConstFold(
      binop
    );
  }
  else if (
    UselessZeroTable.find(op) != UselessZeroTable.end() &&
    opt_right->is<IRLiteral>() &&
    opt_right->as<IRLiteral>()->get() == 0
  ) {
    return opt_left;
  }
  else if (
    UselessZeroTable.find(op) != UselessZeroTable.end() &&
    opt_left->is<IRLiteral>() &&
    opt_left->as<IRLiteral>()->get() == 0
  ) {
    if (op != IRBinOp::Operation::SUB) {
      return opt_right;
    }
  }
  else if (
    op == IRBinOp::Operation::MUL &&
    (
      (opt_left->is<IRLiteral>() && opt_left->as<IRLiteral>()->get() == 1)
      || (opt_right->is<IRLiteral>() && opt_right->as<IRLiteral>()->get() == 1)
    )
   ) {
    return opt_left->is<IRLiteral>() ? opt_right : opt_left;
  }
  else if (
    op == IRBinOp::Operation::MUL && 
    (
      (opt_left->is<IRLiteral>() && opt_left->as<IRLiteral>()->get() == 0)
      || (opt_right->is<IRLiteral>() && opt_right->as<IRLiteral>()->get() == 0)
    )
   ) {
    return new IRLiteral(0);
  }
  else if (
    NoOrderTable.find(op) != NoOrderTable.end() &&
    !opt_left->is<IRBinOp>() &&
    opt_right->is<IRBinOp>()
  ) {
    // Swap the order of the operands to make the right one the non-binop
    IRBinOp *binop = new IRBinOp(std::unique_ptr<IRNode>(opt_right), std::unique_ptr<IRNode>(opt_left), op);
    binop->setInferedType(node->inferType());
    return this->OptimizeBinOp(
      binop,
      canLocFold
    );
  }
  else if (
    op == IRBinOp::Operation::MUL &&
    opt_right->is<IRLiteral>() &&
    pow2(opt_right->as<IRLiteral>()->get())
  ) {
    // pow2 mul into shl
    IRBinOp *binop = new IRBinOp(
      std::unique_ptr<IRNode>(opt_left),
      std::unique_ptr<IRNode>(new IRLiteral(opt_right->as<IRLiteral>()->get() >> 1)),
      IRBinOp::Operation::SHL
    );
    binop->setInferedType(node->inferType());
    return binop;
  }
  else if (
    op == IRBinOp::Operation::DIV &&
    opt_right->is<IRLiteral>() &&
    pow2(opt_right->as<IRLiteral>()->get())
  ) {
    // pow2 div into shr
    IRBinOp *binop = new IRBinOp(
      std::unique_ptr<IRNode>(opt_left),
      std::unique_ptr<IRNode>(new IRLiteral(opt_right->as<IRLiteral>()->get() >> 1)),
      IRBinOp::Operation::SHR
    );
    binop->setInferedType(node->inferType());
    return binop;
  }
  else if (
    op == IRBinOp::Operation::MOD &&
    opt_right->is<IRLiteral>()
  ) {
    int64_t val = opt_right->as<IRLiteral>()->get();
    if (opt_left->is<IRLiteral>()) {
      return new IRLiteral(opt_left->as<IRLiteral>()->get() % val);
    }
    else if (val == 1) {
      return new IRLiteral(0);
    }
    else if ((val&(val-1))==0) {
      IRBinOp *binop = new IRBinOp(
        std::unique_ptr<IRNode>(opt_left),
        std::unique_ptr<IRNode>(new IRLiteral(val - 1)),
        IRBinOp::Operation::AND
      );
      binop->setInferedType(node->inferType());
      return binop;
    }
  }

  if (op == IRBinOp::Operation::L_ASSIGN && opt_left->is<IRLocalRef>()) {
    if (opt_right->is<IRLiteral>()) {
      this->NewLocalValue(opt_left->as<IRLocalRef>()->offset(), opt_right);
    } else if (opt_right->is<IRStringLiteral>()) {
      this->NewLocalValue(opt_left->as<IRLocalRef>()->offset(), opt_right);
    } else {
      this->current_fn->lkv.erase(opt_left->as<IRLocalRef>()->offset());
    }
  }

  IRBinOp *binop = new IRBinOp(std::unique_ptr<IRNode>(opt_left), std::unique_ptr<IRNode>(opt_right), op);
  binop->setInferedType(node->inferType());
  return binop;
}

IRNode *WindOptimizer::OptimizeExpr(IRNode *node, bool canLocFold) {
  if (node->is<IRBinOp>()) {
    return this->OptimizeBinOp(node->as<IRBinOp>(), canLocFold);
  }
  return this->OptimizeNode(node, canLocFold);
}

IRNode *WindOptimizer::OptimizeLDecl(IRVariableDecl *local_decl, bool canLocFold) {
  if (this->current_fn->fn->flags & PURE_STACK) {
    return local_decl;
  }
  assert (this->current_fn != nullptr);
  if (!this->current_fn->fn->isUsed(local_decl->local())) {
    // TODO: Shift stack
    return nullptr;
  }
  IRNode *opt_value = nullptr;
  if (local_decl->value()) {
    opt_value = this->OptimizeExpr(local_decl->value(), canLocFold);
  }
  if (opt_value->is<IRLiteral>() || opt_value->is<IRStringLiteral>()) {
    this->NewLocalValue(local_decl->local()->offset(), opt_value);
  }
  IRVariableDecl *opt_local_decl = new IRVariableDecl(
    local_decl->local(),
    opt_value
  );
  return opt_local_decl;
}

IRNode *WindOptimizer::OptimizeFnCall(IRFnCall *fn_call, bool canLocFold) {
  std::vector<std::unique_ptr<IRNode>> opt_args;
  for (int i=0;i<fn_call->args().size();i++) {
    std::unique_ptr<IRNode> opt_arg = std::unique_ptr<IRNode>(this->OptimizeExpr(fn_call->args()[i].get(), canLocFold));
    opt_args.push_back(std::move(opt_arg));
  }
  IRFnCall *opt_fn_call = new IRFnCall(
    fn_call->name(),
    std::move(opt_args),
    fn_call->getRef()
  );
  return opt_fn_call;
}

bool WindOptimizer::canInline(IRNode *node) {
  if (node->is<IRVariableDecl>()) {
    return false;
  }
  else if (node->is<IRLooping>()) {
    return false;
  }
  else if (node->is<IRFnCall>()) {
    return false;
  }
  else if (node->is<IRBinOp>()) {
    IRBinOp *binop = node->as<IRBinOp>();
    return this->canInline((IRNode*)binop->left()) && this->canInline((IRNode*)binop->right());
  }
  else if (node->is<IRTypeCast>()) {
    IRTypeCast *type_cast = node->as<IRTypeCast>();
    return this->canInline(type_cast->getValue());
  }
  return true;
}

IRNode *WindOptimizer::OptimizeFunction(IRFunction *fn) {
  IRFunction *fn_copy = fn->clone();
  bool can_inline=true;

  for (auto &node : fn->body()->get()) {
    if (node->is<IRRet>()) { can_inline = true; break; } // single return statement, can inline
    if (!this->canInline(node.get())) {
      can_inline = false;
      break;
    }
  }

  if (can_inline) {
    // clear stack usage
    fn_copy->ignore_stack_abi = true;
    fn_copy->stack_size = 0;
  }
  return fn_copy;
}

IRNode *WindOptimizer::OptimizeBranching(IRBranching *branch) {
  std::vector<IRBranch> opt_branches;
  for (auto &branch : branch->getBranches()) {
    IRNode *opt_cond = this->OptimizeExpr(branch.condition.get(), false);
    IRBody *opt_body = new IRBody({});
    for (auto &node : branch.body->as<IRBody>()->get()) {
      std::unique_ptr<IRNode> opt_node = std::unique_ptr<IRNode>(this->OptimizeNode(node.get(), false));
      if (opt_node) {
        *opt_body += std::move(opt_node);
      }
    }
    opt_branches.push_back(
      IRBranch({
        std::unique_ptr<IRNode>(opt_cond),
        std::unique_ptr<IRBody>(opt_body)
      })
    );
  }
  IRBody *opt_else = nullptr;
  if (branch->getElseBranch()) {
    opt_else = new IRBody({});
    for (auto &node : branch->getElseBranch()->get()) {
      std::unique_ptr<IRNode> opt_node = std::unique_ptr<IRNode>(this->OptimizeNode(node.get(), false));
      if (opt_node) {
        *opt_else += std::move(opt_node);
      }
    }
  }
  IRBranching *opt_branching = new IRBranching(
    opt_branches
  );
  if (opt_else) {
    opt_branching->setElseBranch(opt_else);
  }
  return opt_branching;
}

IRNode *WindOptimizer::OptimizeLooping(IRLooping *loop) {
  IRNode *opt_cond = this->OptimizeExpr(loop->getCondition(), false);
  IRBody *opt_body = new IRBody({});
  for (auto &node : loop->getBody()->get()) {
    std::unique_ptr<IRNode> opt_node = std::unique_ptr<IRNode>(this->OptimizeNode(node.get(), false));
    if (opt_node) {
      *opt_body += std::move(opt_node);
    }
  }
  IRLooping *opt_loop = new IRLooping();
  opt_loop->setBody(opt_body);
  opt_loop->setCondition(opt_cond);
  return opt_loop;
}

IRNode *WindOptimizer::OptimizeGenIndexing(IRGenericIndexing *indexing, bool canLocFold) {
  IRNode *opt_base = this->OptimizeExpr(indexing->getBase(), canLocFold);
  IRNode *opt_index = this->OptimizeExpr(indexing->getIndex(), canLocFold);
  IRGenericIndexing *opt_indexing = new IRGenericIndexing(
    opt_base,
    opt_index
  );
  return opt_indexing;
}

IRNode *WindOptimizer::OptimizePtrGuard(IRPtrGuard *ptr_guard, bool canLocFold) {
  IRNode *opt_value = this->OptimizeExpr(ptr_guard->getValue(), canLocFold);
  IRPtrGuard *opt_ptr_guard = new IRPtrGuard(opt_value);
  return opt_ptr_guard;
}

IRNode *WindOptimizer::OptimizeTypeCast(IRTypeCast *type_cast, bool canLocFold) {
  IRNode *opt_value = this->OptimizeExpr(type_cast->getValue(), canLocFold);
  IRTypeCast *opt_type_cast = new IRTypeCast(
    opt_value,
    type_cast->inferType()
  );
  return opt_type_cast;
}

IRNode *WindOptimizer::OptimizeTryCatch(IRTryCatch *try_catch, bool canLocFold) {
  IRBody *opt_try = new IRBody({});
  for (auto &node : try_catch->getTryBody()->get()) {
    std::unique_ptr<IRNode> opt_node = std::unique_ptr<IRNode>(this->OptimizeNode(node.get(), false));
    if (opt_node) {
      *opt_try += std::move(opt_node);
    }
  }
  std::map<HandlerType, IRBody*> opt_handlers;
  for (auto &handler : try_catch->getHandlerMap()) {
    IRBody *opt_handler = new IRBody({});
    for (auto &node : handler.second->get()) {
      std::unique_ptr<IRNode> opt_node = std::unique_ptr<IRNode>(this->OptimizeNode(node.get(), false));
      if (opt_node) {
        *opt_handler += std::move(opt_node);
      }
    }
    opt_handlers[handler.first] = opt_handler;
  }
  IRBody *opt_finally = nullptr;
  if (try_catch->getFinallyBody()) {
    opt_finally = new IRBody({});
    for (auto &node : try_catch->getFinallyBody()->get()) {
      std::unique_ptr<IRNode> opt_node = std::unique_ptr<IRNode>(this->OptimizeNode(node.get(), false));
      if (opt_node) {
        *opt_finally += std::move(opt_node);
      }
    }
  }
  IRTryCatch *opt_try_catch = new IRTryCatch(
    opt_try,
    opt_finally,
    opt_handlers
  );
  return opt_try_catch;
}

IRNode *WindOptimizer::OptimizeArgDecl(IRArgDecl *arg_decl, bool canLocFold) {
  if (this->current_fn->fn->isUsed(arg_decl->local())) {
    return arg_decl;
  }
  // TODO: Shift stack
  return nullptr;
}

IRNode *WindOptimizer::OptimizeNode(IRNode *node, bool canLocFold) {
  if (node->is<IRRet>()) {
    IRRet *ret = node->as<IRRet>();
    IRRet *opt_ret = new IRRet(
      std::unique_ptr<IRNode>(this->OptimizeExpr((IRNode*)ret->get(), canLocFold))
    );
    return opt_ret;
  }
  else if (node->is<IRVariableDecl>()) {
    return this->OptimizeLDecl(node->as<IRVariableDecl>(), canLocFold);
  }
  else if (node->is<IRArgDecl>()) {
    return this->OptimizeArgDecl(node->as<IRArgDecl>(), canLocFold);
  }
  else if (node->is<IRBranching>()) {
    return this->OptimizeBranching(node->as<IRBranching>());
  }
  else if (node->is<IRLooping>()) {
    return this->OptimizeLooping(node->as<IRLooping>());
  }
  else if (node->is<IRTryCatch>()) {
    return this->OptimizeTryCatch(node->as<IRTryCatch>(), canLocFold);
  }
  else if (node->is<IRBinOp>()) {
    return this->OptimizeBinOp(node->as<IRBinOp>(), canLocFold);
  }
  else if (node->is<IRFnCall>()) {
    return this->OptimizeFnCall(node->as<IRFnCall>(), canLocFold);
  }
  else if (node->is<IRGenericIndexing>()) {
    return this->OptimizeGenIndexing(node->as<IRGenericIndexing>(), canLocFold);
  }
  else if (node->is<IRPtrGuard>()) {
    return this->OptimizePtrGuard(node->as<IRPtrGuard>(), canLocFold);
  }
  else if (node->is<IRTypeCast>()) {
    return this->OptimizeTypeCast(node->as<IRTypeCast>(), canLocFold);
  }
  else if (node->is<IRLocalRef>()) {
    if (canLocFold && this->GetLocalValue(node->as<IRLocalRef>()->offset())) {
      return this->GetLocalValue(node->as<IRLocalRef>()->offset());
    }
  }
  else if (node->is<IRLocalAddrRef>()) {
    IRLocalAddrRef *addr = node->as<IRLocalAddrRef>();
    if (addr->isIndexed()) {
      return new IRLocalAddrRef(
        addr->offset(),
        addr->datatype(),
        this->OptimizeExpr(addr->getIndex(), canLocFold)
      );
    }
    return node;
  }
  else if (node->is<IRGenericIndexing>()) {
    IRGenericIndexing *indexing = node->as<IRGenericIndexing>();
    return new IRGenericIndexing(
      this->OptimizeExpr(indexing->getBase(), canLocFold),
      this->OptimizeExpr(indexing->getIndex(), canLocFold)
    );
  }
  return node;
}

void WindOptimizer::OptimizeBody(IRBody *body, IRFunction *parent) {
  IRBody *tbody = parent->body();
  for (auto &node : body->get()) {
    IRNode *raw_opt_node = this->OptimizeNode(node.get(), true);
    if (raw_opt_node) {
      std::unique_ptr<IRNode> opt_node = std::unique_ptr<IRNode>(raw_opt_node);
      *tbody += std::move(opt_node);
    }
  }
}

void WindOptimizer::optimize() {
  for (std::string &fn_name : this->program->getDefFns()) {
    this->emission->addDefFn(fn_name);
  }
  for (auto &node : this->program->get()) {
    if (node->is<IRFunction>()) {
      IRFunction *fn = (IRFunction*)node->as<IRFunction>();
      IRFunction *fn_copy = (IRFunction*)this->OptimizeFunction(fn);
      *this->emission += std::unique_ptr<IRNode>(fn_copy);
      this->current_fn = new WindOptimizer::FunctionDesc({});
      this->current_fn->fn = fn_copy;
      this->OptimizeBody(fn->body(), fn_copy);
    } else {
      *this->emission += std::unique_ptr<IRNode>(this->OptimizeNode(node.get(), true));
    }
  }
}

IRBody *WindOptimizer::get() {
  return this->emission;
}

void WindOptimizer::NewLocalValue(int16_t local, IRNode *value) {
  this->current_fn->lkv[local] = value;
}

IRNode *WindOptimizer::GetLocalValue(int16_t local) {
  if (this->current_fn->lkv.find(local) != this->current_fn->lkv.end()) {
    //return this->current_fn->lkv[local];
    // set size and signed
    IRNode *value = this->current_fn->lkv[local];
    switch (value->type()) {
      case IRNode::NodeType::LITERAL: {
        IRLocalRef *local_ref;
        for (auto &local : this->current_fn->fn->locals()) {
          if (local->offset() == local.get()->offset()) {
            local_ref = local.get();
            break;
          }
        }
        value->as<IRLiteral>()->setSigned(local_ref->datatype()->isSigned());
        //value->as<IRLiteral>()->setSize(local_ref->datatype()->moveSize());
      }
      default: return value;
    }
  }
  return nullptr;
}