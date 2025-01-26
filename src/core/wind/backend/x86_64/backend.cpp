/**
 * @file backend.cpp
 * @brief Implementation of the WindEmitter class for x86_64 backend.
 */

#include <wind/generation/IR.h>
#include <wind/backend/writer/writer.h>
#include <wind/backend/x86_64/backend.h>
#include <wind/bridge/flags.h>
#include <wind/processing/utils.h>
#include <wind/backend/interface/gas.h>
#include <stdexcept>
#include <fstream>


void WindEmitter::ProcessTop(IRNode *node) {
    switch (node->type()) {
        case IRNode::NodeType::FUNCTION:
            this->ProcessFunction(node->as<IRFunction>());
            break;
        case IRNode::NodeType::GLOBAL_DECL:
            this->ProcessGlobal(node->as<IRGlobalDecl>());
            break;
        default:
            throw std::runtime_error("Invalid top-level node type(" + std::to_string((uint8_t)node->type()) + "): Report to maintainer!");
    }
}

void WindEmitter::EmitInAsm(IRInlineAsm *asmn) {
    this->writer->Write(asmn->code());
}

void WindEmitter::ProcessStatement(IRNode *node) {
    switch (node->type()) {
        case IRNode::NodeType::RET:
            this->EmitReturn(node->as<IRRet>());
            break;
        case IRNode::NodeType::LOCAL_DECL:
            for (IRVariableDecl *decl=node->as<IRVariableDecl>(); decl->value();) {
                this->EmitIntoLocalVar(decl->local(), decl->value());
                break;
            }
            break;
        case IRNode::NodeType::IN_ASM:
            this->EmitInAsm(node->as<IRInlineAsm>());
            break;
        case IRNode::NodeType::BRANCH:
            this->EmitBranch(node->as<IRBranching>());
            break;
        case IRNode::NodeType::LOOP:
            this->EmitLoop(node->as<IRLooping>());
            break;
        case IRNode::NodeType::CONTINUE:
            this->EmitContinue();
            break;
        case IRNode::NodeType::BREAK:
            this->EmitBreak();
            break;
        default: {
            this->EmitExpr(node, x86::Gp::rax);
        }
    }
}

/**
 * @brief Processes the IR program.
 */
void WindEmitter::Process() {
    for (auto &block : program->get()) {
        this->ProcessTop(block.get());
    }
    this->state->EmitStrings();
}


std::string WindEmitter::emitObj(std::string outpath) {
  std::string outemit = this->GetAsm();
  std::string path = generateRandomFilePath("", ".S");
  std::ofstream file(path);
  if (file.is_open()) {
    file << outemit;
    file.close();
  }
  WindGasInterface *gas = new WindGasInterface(path, outpath);
  std::string ret = gas->assemble();
  return ret;
}