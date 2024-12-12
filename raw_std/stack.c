#include <stdio.h>

void __WDcanary_fail(void *ptr, const char *name) {
  puts("[STACK CHECK] *** Integrity check failed ***");
  printf("** BACKTRACE: %s (%p) **\n", name, ptr);
  asm ("mov $60,%rax\nmov $134,%rdi\nsyscall");
}