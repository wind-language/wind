#include <wind/generation/IR.h>
#include <wind/generation/backend.h>
#include <wind/common/debug.h>
#include <wind/generation/gas.h>
#include <asmjit/asmjit.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <math.h>
#include <filesystem>
#include <random>
#include <map>
#include <regex>

const asmjit::x86::Gp SystemVABI[] = {
  asmjit::x86::rdi,
  asmjit::x86::rsi,
  asmjit::x86::rdx,
  asmjit::x86::rcx,
  asmjit::x86::r8,
  asmjit::x86::r9
};

void WindEmitter::InitializeSections() {
  this->text = this->code_holder.textSection();
  this->code_holder.newSection(
    &this->data, ".data", 6,
    asmjit::SectionFlags::kNone,
    1,
    1
  );
  this->code_holder.newSection(
    &this->bss, ".bss", 5,
    asmjit::SectionFlags::kZeroInitialized,
    1,
    2
  );
  this->code_holder.newSection(
    &this->rodata, ".rodata", 8,
    asmjit::SectionFlags::kReadOnly,
    1,
    3
  );
}

WindEmitter::WindEmitter(IRBody *program) : program(program) {
  this->code_holder.init(asmjit::Environment::host());
  this->InitializeSections();
  this->assembler = new asmjit::x86::Assembler(&this->code_holder);
  this->logger = new asmjit::StringLogger();
  this->logger->content().appendFormat(".intel_syntax noprefix\n");
  this->logger->setFlags(asmjit::FormatFlags::kHexOffsets);
  this->assembler->setLogger(this->logger);
  this->assembler->section(this->text);
  this->string_table.table = new std::map<std::string, std::string>();
  this->secHeader();
}

WindEmitter::~WindEmitter() {}

int nextpow2(int n) {
  return pow(2, ceil(log2(n)));
}

void WindEmitter::emitPrologue() {
  if (
    (current_function->isStack() && current_function->used_offsets.size() > 0)
    || current_function->flags & PURE_LOGUE
  ) {
    this->assembler->push(asmjit::x86::rbp);
    this->assembler->mov(asmjit::x86::rbp, asmjit::x86::rsp);
    if (current_function->isCallSub()) {
      this->assembler->sub(asmjit::x86::rsp, 16);
    }
  }
  this->canaryPrologue();
}

void WindEmitter::emitEpilogue() {
  this->canaryEpilogue();
  if (
    (current_function->isStack() && current_function->used_offsets.size() > 0) 
    || current_function->flags & PURE_LOGUE
  ) {
    this->assembler->leave();
  }
  this->assembler->ret();
}

void WindEmitter::emitFunction(IRFunction *fn) {
  if (fn->name() == "main") {
    this->logger->content().appendFormat(".global main\n");
  }
  current_function = fn;
  this->cconv_index = 0;
  this->assembler->bind(this->assembler->newNamedLabel(fn->name().c_str()));
  this->emitPrologue();
  for (auto &statement : fn->body()->get()) {
    this->emitNode(statement.get());
  }
  this->emitEpilogue();
}

asmjit::x86::Gp WindEmitter::adaptReg(asmjit::x86::Gp reg, int size) {
  switch (size) {
    case 1:
      return reg.r8();
    case 2:
      return reg.r16();
    case 4:
      return reg.r32();
    case 8:
      return reg.r64();
    default: {
      throw std::runtime_error("Invalid size");
    }
  } 
}

asmjit::x86::Gp WindEmitter::moveVar(IRLocalRef *local, asmjit::x86::Gp dest) {
  asmjit::x86::Gp reg = adaptReg(dest, local->size());
  this->assembler->mov(reg, asmjit::x86::ptr(asmjit::x86::rbp, -local->offset(), local->size()));
  return reg;
}

void WindEmitter::moveIntoVar(IRLocalRef *local, IRNode *value) {
  if (value->is<IRLiteral>()) {
    IRLiteral *lit = value->as<IRLiteral>();
    this->assembler->mov(
      asmjit::x86::ptr(asmjit::x86::rbp, -local->offset(), local->size()),
      lit->get()
    );
  } else {
    asmjit::x86::Gp reg = this->adaptReg(asmjit::x86::rax, local->size());
    asmjit::x86::Gp src = this->adaptReg(this->emitExpr(value, reg), local->size());
    this->assembler->mov(asmjit::x86::ptr(asmjit::x86::rbp, -local->offset(), local->size()), src);
  }
}

asmjit::x86::Gp WindEmitter::emitBinOp(IRBinOp *bin_op, asmjit::x86::Gp dest) {
  IRNode *right_node = (IRNode*)bin_op->right();
  asmjit::x86::Gp left;
  if (right_node->is<IRBinOp>()) {
    left = this->emitExpr((IRNode*)bin_op->left(), asmjit::x86::r10);
    right_node = new IRRegister(this->emitBinOp(right_node->as<IRBinOp>(), asmjit::x86::rax));
  } else {
    left = this->emitExpr((IRNode*)bin_op->left(), asmjit::x86::rax);
  }

  switch (right_node->type()) {
    case IRNode::NodeType::LITERAL : {
      IRLiteral *lit = right_node->as<IRLiteral>();
      switch (bin_op->operation()) {
        case IRBinOp::Operation::ADD : {
          this->assembler->add(left, lit->get());
          break;
        }
        case IRBinOp::Operation::SUB : {
          this->assembler->sub(left, lit->get());
          break;
        }
        case IRBinOp::Operation::MUL : {
          this->assembler->imul(left, lit->get());
          break;
        }
        case IRBinOp::Operation::DIV : {
          throw std::runtime_error("TODO: Implement division");
          break;
        }
        case IRBinOp::Operation::SHL : {
          this->assembler->shl(left, lit->get());
          break;
        }
        case IRBinOp::Operation::SHR : {
          this->assembler->shr(left, lit->get());
          break;
        }
      }
      if (left != dest) {
        this->assembler->mov(dest, left);
      }
      return left;
    }

    case IRNode::NodeType::FUNCTION_CALL : {
      this->assembler->mov(asmjit::x86::r10, left);
      this->emitExpr(right_node, asmjit::x86::rax);
      switch (bin_op->operation()) {
        case IRBinOp::Operation::ADD : {
          this->assembler->add(asmjit::x86::r10, asmjit::x86::rax);
          break;
        }
        case IRBinOp::Operation::SUB : {
          this->assembler->sub(asmjit::x86::r10, asmjit::x86::rax);
          break;
        }
        case IRBinOp::Operation::MUL : {
          this->assembler->imul(asmjit::x86::r10, asmjit::x86::rax);
          break;
        }
        case IRBinOp::Operation::DIV : {
          throw std::runtime_error("TODO: Implement division");
          break;
        }
        case IRBinOp::Operation::SHL : {
          this->assembler->shl(asmjit::x86::r10, asmjit::x86::rax);
          break;
        }
        case IRBinOp::Operation::SHR : {
          this->assembler->shr(asmjit::x86::r10, asmjit::x86::rax);
          break;
        }
      }
      if (dest.r64() != asmjit::x86::rax) {
        this->assembler->mov(dest, asmjit::x86::rax);
      }
      return dest; // TODO: Return the correct register size
    }

    case IRNode::NodeType::LOCAL_REF : {
      IRLocalRef *local = right_node->as<IRLocalRef>();
      switch (bin_op->operation()) {
        case IRBinOp::Operation::ADD : {
          this->assembler->add(left, asmjit::x86::ptr(asmjit::x86::rbp, -local->offset(), local->size()));
          break;
        }
        case IRBinOp::Operation::SUB : {
          this->assembler->sub(left, asmjit::x86::ptr(asmjit::x86::rbp, -local->offset(), local->size()));
          break;
        }
        case IRBinOp::Operation::MUL : {
          this->assembler->imul(left, asmjit::x86::ptr(asmjit::x86::rbp, -local->offset(), local->size()));
          break;
        }
        case IRBinOp::Operation::DIV : {
          throw std::runtime_error("TODO: Implement division");
          break;
        }
        case IRBinOp::Operation::SHL : {
          this->assembler->shl(left, this->moveVar(local, asmjit::x86::r10));
          break;
        }
        case IRBinOp::Operation::SHR : {
          this->assembler->shr(left, this->moveVar(local, asmjit::x86::r10));
          break;
        }
      }
      if (left != dest) {
        this->assembler->mov(dest, left);
      }
      return left;
    }

    case IRNode::NodeType::REGISTER : {
      asmjit::x86::Gp right = right_node->as<IRRegister>()->reg();
      int biggest_size = std::max(left.size(), right.size());
      left = this->adaptReg(left, biggest_size);
      right = this->adaptReg(right, biggest_size);
      switch (bin_op->operation()) {
        case IRBinOp::Operation::ADD : {
          this->assembler->add(left, right);
          break;
        }
        case IRBinOp::Operation::SUB : {
          this->assembler->sub(left, right);
          break;
        }
        case IRBinOp::Operation::MUL : {
          this->assembler->imul(left, right);
          break;
        }
        case IRBinOp::Operation::DIV : {
          throw std::runtime_error("TODO: Implement division");
          break;
        }
        case IRBinOp::Operation::SHL : {
          this->assembler->shl(left, right);
          break;
        }
        case IRBinOp::Operation::SHR : {
          this->assembler->shr(left, right);
          break;
        }
      }
      if (left != dest) {
        this->assembler->mov(dest, left);
      }
      return left;
    }

    default: {
      std::cout << "Unknown node type" << std::endl;
      break;
    }
  }
  return dest;
}

asmjit::x86::Gp WindEmitter::emitLiteral(IRLiteral *lit, asmjit::x86::Gp dest) {
  long long value = lit->get();
  this->assembler->mov(dest, value);
  return dest;
}

IRFunction *WindEmitter::FindFunction(std::string name) {
  for (auto &statement : this->program->get()) {
    if (statement->type() == IRNode::NodeType::FUNCTION) {
      IRFunction *fn = statement->as<IRFunction>();
      if (fn->name() == name) {
        return fn;
      }
    }
  }
  return nullptr;
}

void WindEmitter::SolveCArg(IRNode *arg, int type) {
  if (this->cconv_index < 6) {
    this->emitExpr(arg, this->adaptReg(SystemVABI[this->cconv_index], type));
  } else {
    asmjit::x86::Gp reg = this->emitExpr(arg, this->adaptReg(asmjit::x86::rax, type));
    this->assembler->push(reg);
  }
  this->cconv_index++;
}

void WindEmitter::emitFunctionCall(IRFnCall *fn_call) {
  this->cconv_index = 0;
  IRFunction *fn = this->FindFunction(fn_call->name());
  if (!fn) {
    throw std::runtime_error("Function not found");
  }
  for (size_t i = 0; i < fn_call->args().size(); i++) {
    this->SolveCArg(fn_call->args()[i].get(), fn->GetArgSize(i));
  }
  this->assembler->call(this->assembler->labelByName(fn->name().c_str()));
}

asmjit::x86::Gp WindEmitter::emitExpr(IRNode *node, asmjit::x86::Gp dest) {
  switch (node->type()) {

    case IRNode::NodeType::LITERAL : {
      IRLiteral *lit = node->as<IRLiteral>();
      return this->emitLiteral(lit, dest);
    }

    case IRNode::NodeType::LOCAL_REF : {
      IRLocalRef *local = node->as<IRLocalRef>();
      return this->moveVar(local, dest);
    }

    case IRNode::NodeType::FUNCTION_CALL : {
      IRFnCall *fn_call = node->as<IRFnCall>();
      this->emitFunctionCall(fn_call);
      break;
    }

    case IRNode::NodeType::BIN_OP : {
      IRBinOp *bin_op = node->as<IRBinOp>();
      return this->emitBinOp(bin_op, dest);
    }

    default: {
      std::cout << "Unknown node type" << std::endl;
      break;
    }

  }
  return dest;
}

void WindEmitter::SolveArg(IRArgDecl *decl) {
  IRLocalRef *local = decl->local();
  if (this->cconv_index < 6) {
    this->assembler->mov(
      asmjit::x86::ptr(asmjit::x86::rbp, -local->offset(), local->size()),
      this->adaptReg(SystemVABI[this->cconv_index], local->size())
    );
  } else {
    asmjit::x86::Gp rax = this->adaptReg(asmjit::x86::rax, local->size());
    this->assembler->mov(
      rax, asmjit::x86::ptr(asmjit::x86::rbp, (this->cconv_index-6)*8, local->size())
    );
    this->assembler->mov(
      asmjit::x86::ptr(asmjit::x86::rbp, -local->offset(), local->size()), rax
    );
  }
  this->cconv_index++;
}

void WindEmitter::emitNode(IRNode *node) {
  switch (node->type()) {
    case IRNode::NodeType::FUNCTION : {
      this->emitFunction(node->as<IRFunction>());
      break;
    }
    case IRNode::NodeType::RET : {
      IRRet *ret = node->as<IRRet>();
      this->emitExpr(ret->get(), asmjit::x86::rax);
      break;
    }
    case IRNode::NodeType::LOCAL_DECL : {
      IRLocalDecl *local_decl = node->as<IRLocalDecl>();
      if (local_decl->value()) {
        this->moveIntoVar(local_decl->local(), local_decl->value());
      }
      break;
    }
    case IRNode::NodeType::ARG_DECL : {
      IRArgDecl *arg_decl = node->as<IRArgDecl>();
      this->SolveArg(arg_decl);
      break;
    }
    default: {
      std::cout << "Unknown node type" << std::endl;
      break;
    }
  }
}

void cleanLoggerContent(std::string& outemit) {
  std::regex ptrPattern(R"(\b\w+word\s+ptr\b\s)");
  outemit = std::regex_replace(outemit, ptrPattern, "");
  std::regex sectionPattern(R"(\.section\s+([^\s]+)\s+\{\#[0-9]+\})");
  outemit = std::regex_replace(outemit, sectionPattern, ".section $1");
}

std::string generateRandomFilePath(const std::string& directory, const std::string& extension) {
    std::string tempDir = directory.empty() ? std::filesystem::temp_directory_path().string() : directory;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(100000, 999999);
    std::string randomFileName = "tmp" + std::to_string(dist(gen)) + extension;
    return tempDir + "/" + randomFileName;
}

std::string WindEmitter::newRoString(std::string str) {
  std::string label = ".RLC" + std::to_string(this->rostring_i);
  this->string_table.table->insert(std::pair<std::string, std::string>(label, str));
  this->rostring_i++;
  return label;
}

std::string WindEmitter::emit() {
  for (auto &statement : this->program->get()) {
    this->emitNode(statement.get());
  }

  this->code_holder.flatten();
  this->code_holder.resolveUnresolvedLinks();

  this->assembler->section(this->rodata);
  for (auto &entry : *this->string_table.table) {
    this->assembler->bind(this->assembler->newNamedLabel(entry.first.c_str()));
    this->logger->content().appendFormat(".string \"%s\"\n", entry.second.c_str());
  }

  std::string outemit = this->logger->content().data();\
  cleanLoggerContent(outemit);
  std::string path = generateRandomFilePath("", ".S");
  std::ofstream file(path);
  if (file.is_open()) {
    file << outemit;
    file.close();
  }
  WindGasInterface *gas = new WindGasInterface(path);
  return gas->assemble();
}