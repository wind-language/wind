@include [
    "#types.w"
]

@extern func puts(str: string) : void;
@extern func abort() : void;

@pub @pure[stchk] func __WDH_sum_overflow() : void {
  puts("*** [ sum overflow ] ***");
  asm {
    mov rdi, r15; 
  }
  puts();
  abort();
}