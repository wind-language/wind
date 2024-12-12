#include <wind/generation/backend.h>
#include <asmjit/asmjit.h>

#define CANARY_FS_OFFSET 0x40

void WindEmitter::secHeader() {
  //this->logger->content().appendFormat(".extern __WDcanary_fail\n");
}

void WindEmitter::canaryPrologue() {
  /* uint8_t movmcode[] = { // Disas of mov rax, fs:40h
    0x64, 0x48, 0xa1, 0x00,
    00, 00, 00, 00, 00, 00, 00
  }; */
  this->logger->content().appendFormat("mov rax, fs:0x40\n");
  this->assembler->mov(asmjit::x86::ptr(asmjit::x86::rbp, -8, 8), asmjit::x86::rax);
  this->assembler->xor_(asmjit::x86::rax, asmjit::x86::rax);
}

void WindEmitter::canaryEpilogue() {
  this->assembler->mov(asmjit::x86::rdx, asmjit::x86::ptr(asmjit::x86::rbp, -8, 8));
  this->logger->content().appendFormat("lea rdi, [rip+%s]\n", this->current_function->name().c_str());
  this->logger->content().appendFormat("lea rsi, [rip+%s]\n", this->newRoString(this->current_function->name()).c_str());
  this->logger->content().appendFormat("sub rdx, fs:0x40\njne __WDcanary_fail\n");
}