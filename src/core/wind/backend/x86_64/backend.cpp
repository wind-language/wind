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

Reg WindEmitter::CastReg(Reg reg, uint8_t size) {
    if (reg.size == size) return reg;
    if (size == 1) {
        return {reg.id, 1, reg.type};
    } else if (size == 2) {
        return {reg.id, 2, reg.type};
    } else if (size == 4) {
        return {reg.id, 4, reg.type};
    } else if (size == 8) {
        return {reg.id, 8, reg.type};
    } else {
        throw std::runtime_error("Invalid register size");
    }
}

void WindEmitter::ProcessTop(IRNode *node) {
    switch (node->type()) {
        case IRNode::NodeType::GLOBAL_DECL:
            this->ProcessGlobalDecl(node->as<IRGlobalDecl>());
            break;
        case IRNode::NodeType::FUNCTION:
            this->ProcessFunction(node->as<IRFunction>());
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
            this->EmitLocDecl(node->as<IRVariableDecl>());
            break;
        case IRNode::NodeType::IN_ASM:
            this->EmitInAsm(node->as<IRInlineAsm>());
            break;
        case IRNode::NodeType::LOOP:
            this->EmitLoop(node->as<IRLooping>());
            break;
        case IRNode::NodeType::BRANCH: 
            this->EmitBranch(node->as<IRBranching>());
            break;
        case IRNode::NodeType::BREAK:
            this->writer->jmp(this->writer->LabelById(this->c_flow_desc->end));
            break;
        case IRNode::NodeType::CONTINUE:
            this->writer->jmp(this->writer->LabelById(this->c_flow_desc->start));
            break;
        case IRNode::NodeType::TRY_CATCH:
            this->EmitTryCatch(node->as<IRTryCatch>());
            break;
        default:
            this->EmitExpr(node, Reg({0, 8, Reg::GPR}));
    }
}

/**
 * @brief Processes the IR program.
 */
void WindEmitter::Process() {
    this->dataSection = this->writer->NewSection(".data");
    this->rodataSection = this->writer->NewSection(".rodata");
    this->textSection = this->writer->NewSection(".text");
    for (auto &block : program->get()) {
        this->ProcessTop(block.get());
    }
    this->writer->BindSection(this->rodataSection);
    for (int i=0;i<this->rostr_i;i++) {
        this->writer->BindLabel(this->writer->NewLabel(".ros"+std::to_string(i)));
        this->writer->String(rostrs[i]);
    }
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
  gas->addFlag("-O3");
  std::string ret = gas->assemble();
  return ret;
}