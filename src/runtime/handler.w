@include "#types.w"

@extern func puts(str: string) : void;
@extern func printf(fmt: string, ...) : void;
@extern func abort() : void;

@pub @pure[stchk] func __WDH_sum_overflow() : void {
  puts("*** [ sum overflow ] ***");
  asm {
    mov rsi, r15; 
  }
  printf("Sum overflow encountered in function '%s'\n");
  abort();
}

@pub @pure[stchk] func __WDH_sub_overflow() : void {
  puts("*** [ sub overflow ] ***");
  asm {
    mov rsi, r15;
  }
  printf("Sub overflow encountered in function '%s'\n");
  abort();
}

@pub @pure[stchk] func __WDH_mul_overflow() : void {
  puts("*** [ mul overflow ] ***");
  asm {
    mov rsi, r15;
  }
  printf("Mul overflow encountered in function '%s'\n");
  abort();
}

@pub @pure[stchk] func __WDH_div_overflow() : void {
  puts("*** [ div overflow ] ***");
  asm {
    mov rsi, r15;
  }
  printf("Div overflow encountered in function '%s'\n");
  abort();
}

@pub @pure[stchk] func __WDH_out_of_bounds() : void {
  puts("*** [ out of bounds ] ***");
  asm {
    mov rsi, r15;
  }
  printf("Array indexing out of bounds encountered in function '%s'\n");
  abort();
}