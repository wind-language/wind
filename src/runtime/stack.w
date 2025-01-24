@include [
  "#libc.wi"
]

@pub @pure[stchk no_mangle] func __WD_canary_fail() : void {
  puts("*** [ canary fail ] ***");
  asm {
    mov rsi, r15;
  }
  printf("Canary fail encountered in function '%s'\n");
  abort();
}