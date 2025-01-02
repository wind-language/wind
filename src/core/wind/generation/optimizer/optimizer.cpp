#include <wind/generation/IR.h>
#include <wind/generation/optimizer.h>
#include <wind/bridge/opt_flags.h>
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
    /* case IRBinOp::Operation::MOD:
      return new IRLiteral(left % right);
    case IRBinOp::Operation::AND:
      return new IRLiteral(left & right);
    case IRBinOp::Operation::OR:
      return new IRLiteral(left | right);
    case IRBinOp::Operation::XOR:
      return new IRLiteral(left ^ right);
    case IRBinOp::Operation::SHL:
      return new IRLiteral(left << right);
    case IRBinOp::Operation::SHR:
      return new IRLiteral(left >> right);
    case IRBinOp::Operation::EQ:
      return new IRLiteral(left == right);
    case IRBinOp::Operation::NE:
      return new IRLiteral(left != right);
    case IRBinOp::Operation::LT:
      return new IRLiteral(left < right);
    case IRBinOp::Operation::GT:
      return new IRLiteral(left > right);
    case IRBinOp::Operation::LE:
      return new IRLiteral(left <= right);
    case IRBinOp::Operation::GE:
      return new IRLiteral(left >= right); */
    default:
      return nullptr;
  }
}

bool pow2(long long n) {
  return (n & (n - 1)) == 0;
}


IRNode *WindOptimizer::OptimizeBinOp(IRBinOp *node) {
  IRNode *left = (IRNode*)node->left();
  IRNode *right = (IRNode*)node->right();
  IRBinOp::Operation op = node->operation();
  if (this->current_fn->flags & PURE_EXPR) {
    return new IRBinOp(std::unique_ptr<IRNode>(left), std::unique_ptr<IRNode>(right), op);
  }
  IRNode *opt_left = this->OptimizeExpr(left);
  IRNode *opt_right = this->OptimizeExpr(right);
  
  if (opt_left->is<IRLiteral>() && opt_right->is<IRLiteral>()) {
    return this->OptimizeConstFold(node);
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
    return opt_right;
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
    return this->OptimizeBinOp(
      new IRBinOp(std::unique_ptr<IRNode>(opt_right), std::unique_ptr<IRNode>(opt_left), op)
    );
  }
  else if (
    op == IRBinOp::Operation::MUL &&
    opt_right->is<IRLiteral>() &&
    pow2(opt_right->as<IRLiteral>()->get())
  ) {
    // pow2 mul into shl
    return new IRBinOp(
      std::unique_ptr<IRNode>(opt_left),
      std::unique_ptr<IRNode>(new IRLiteral(opt_right->as<IRLiteral>()->get() >> 1)),
      IRBinOp::Operation::SHL
    );
  }
  else if (
    op == IRBinOp::Operation::DIV &&
    opt_right->is<IRLiteral>() &&
    pow2(opt_right->as<IRLiteral>()->get())
  ) {
    // pow2 div into shr
    return new IRBinOp(
      std::unique_ptr<IRNode>(opt_left),
      std::unique_ptr<IRNode>(new IRLiteral(opt_right->as<IRLiteral>()->get() >> 1)),
      IRBinOp::Operation::SHR
    );
  }
  else if (
    op == IRBinOp::Operation::MOD &&
    opt_right->is<IRLiteral>()
  ) {
    if (opt_left->is<IRLiteral>()) {
      return new IRLiteral(opt_left->as<IRLiteral>()->get() % opt_right->as<IRLiteral>()->get());
    }
    else if (opt_right->as<IRLiteral>()->get() == 1) {
      return new IRLiteral(0);
    }
    else if (opt_right->as<IRLiteral>()->get() == 2) {
      return new IRBinOp(
        std::unique_ptr<IRNode>(opt_left),
        std::unique_ptr<IRNode>(new IRLiteral(1)),
        IRBinOp::Operation::AND
      );
    }
  }
  IRBinOp *binop = new IRBinOp(std::unique_ptr<IRNode>(opt_left), std::unique_ptr<IRNode>(opt_right), op);
  return binop;
}

IRNode *WindOptimizer::OptimizeExpr(IRNode *node) {
  if (node->is<IRBinOp>()) {
    return this->OptimizeBinOp(node->as<IRBinOp>());
  }
  return node;
}

IRNode *WindOptimizer::OptimizeLDecl(IRVariableDecl *local_decl) {
  if (this->current_fn->flags & PURE_STACK) {
    return local_decl;
  }
  assert (this->current_fn != nullptr);
  if (!this->current_fn->isUsed(local_decl->local())) {
    this->current_fn->stack_size -= local_decl->local()->datatype()->memSize();
    return nullptr;
  }
  IRNode *opt_value = nullptr;
  if (local_decl->value()) {
    opt_value = this->OptimizeExpr(local_decl->value());
  }
  IRVariableDecl *opt_local_decl = new IRVariableDecl(
    local_decl->local(),
    opt_value
  );
  return opt_local_decl;
}

IRNode *WindOptimizer::OptimizeFnCall(IRFnCall *fn_call) {
  std::vector<IRNode*> opt_args;
  for (int i=0;i<fn_call->args().size();i++) {
    std::unique_ptr<IRNode> opt_arg = std::unique_ptr<IRNode>(this->OptimizeExpr(fn_call->args()[i].get()));
    fn_call->replaceArg(i, std::move(opt_arg));
    std::cerr << (int)fn_call->args()[i]->type() << std::endl;
  }
  return fn_call;
}

IRNode *WindOptimizer::OptimizeFunction(IRFunction *fn) {
  IRFunction *fn_copy = fn->clone();
  bool has_loop=false;

  for (auto &node : fn->body()->get()) {
    if (node->is<IRLooping>()) {
      has_loop = true;
      break;
    }
  }

  if (fn->ArgNum()==1 && !has_loop) {
    // clear stack usage
    fn_copy->stack_size -= fn->GetArgType(0)->memSize();
    fn_copy->ignore_stack_abi = true;
  }
  return fn_copy;
}

IRNode *WindOptimizer::OptimizeBranching(IRBranching *branch) {
  std::vector<IRBranch> opt_branches;
  for (auto &branch : branch->getBranches()) {
    IRNode *opt_cond = this->OptimizeExpr(branch.condition.get());
    IRBody *opt_body = new IRBody({});
    for (auto &node : branch.body->as<IRBody>()->get()) {
      std::unique_ptr<IRNode> opt_node = std::unique_ptr<IRNode>(this->OptimizeNode(node.get()));
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
      std::unique_ptr<IRNode> opt_node = std::unique_ptr<IRNode>(this->OptimizeNode(node.get()));
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
  IRNode *opt_cond = this->OptimizeExpr(loop->getCondition());
  IRBody *opt_body = new IRBody({});
  for (auto &node : loop->getBody()->get()) {
    std::unique_ptr<IRNode> opt_node = std::unique_ptr<IRNode>(this->OptimizeNode(node.get()));
    if (opt_node) {
      *opt_body += std::move(opt_node);
    }
  }
  IRLooping *opt_loop = new IRLooping();
  opt_loop->setBody(opt_body);
  opt_loop->setCondition(opt_cond);
  return opt_loop;
}

IRNode *WindOptimizer::OptimizeNode(IRNode *node) {
  if (node->is<IRRet>()) {
    IRRet *ret = node->as<IRRet>();
    IRRet *opt_ret = new IRRet(
      std::unique_ptr<IRNode>(this->OptimizeExpr((IRNode*)ret->get()))
    );
    return opt_ret;
  }
  else if (node->is<IRVariableDecl>()) {
    return this->OptimizeLDecl(node->as<IRVariableDecl>());
  }
  else if (node->is<IRBranching>()) {
    return this->OptimizeBranching(node->as<IRBranching>());
  }
  else if (node->is<IRLooping>()) {
    return this->OptimizeLooping(node->as<IRLooping>());
  }
  else if (node->is<IRBinOp>()) {
    return this->OptimizeBinOp(node->as<IRBinOp>());
  }
  /* else if (node->is<IRFnCall>()) {
    return this->OptimizeFnCall(node->as<IRFnCall>());
  } */
  return node;
}

void WindOptimizer::OptimizeBody(IRBody *body, IRFunction *parent) {
  IRBody *tbody = parent->body();
  for (auto &node : body->get()) {
    std::unique_ptr<IRNode> opt_node = std::unique_ptr<IRNode>(this->OptimizeNode(node.get()));
    if (opt_node) {
      *tbody += std::move(opt_node);
    }
  }
}

void WindOptimizer::optimize() {
  for (auto &node : this->program->get()) {
    if (node->is<IRFunction>()) {
      IRFunction *fn = (IRFunction*)node->as<IRFunction>();
      IRFunction *fn_copy = (IRFunction*)this->OptimizeFunction(fn);
      *this->emission += std::unique_ptr<IRNode>(fn_copy);
      this->current_fn = fn_copy;
      this->OptimizeBody(fn->body(), fn_copy);
    } else {
      *this->emission += std::unique_ptr<IRNode>(this->OptimizeNode(node.get()));
    }
  }
}

IRBody *WindOptimizer::get() {
  return this->emission;
}