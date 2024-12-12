.section .text
.global _start
.extern main

__WDcanary_gen:
  rdtsc
  mov %rdi, %fs:0x40
  ret

_start:
  // generate canary
  call __WDcanary_gen
  call main
  mov %rax,%rdi
  mov $60,%rax
  syscall
