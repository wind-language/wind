@include [
    "#types.w"
]

@extern func puts(str: string) : void;
@extern func printf(fmt: string, ...) : void;
@extern func abort() : void;

@pub @pure[stchk] func __WD_canary_fail() : void {
  puts("*** [ canary fail ] ***");
  asm {
    mov rsi, r15;
  }
  printf("Canary fail encountered in function '%s'\n");
  abort();
}