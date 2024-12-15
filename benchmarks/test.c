#include <stdio.h>
#include <string.h>

int main() {
  char localVar[32];
  memcpy(localVar, "Hello\0", 6);
  printf("Mem: %s\n", localVar);
  return 0;
}
