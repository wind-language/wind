@include [
  "#libc.wi"
]

@pub @pure[stchk no_mangle] func __WDH_sum_overflow(metadata: string) : void {
  puts("*** [ sum overflow ] ***");
  printf("Sum overflow encountered in function '%s'\n", metadata);
  abort();
}

@pub @pure[stchk no_mangle] func __WDH_sub_overflow(metadata: string) : void {
  puts("*** [ sub overflow ] ***");
  printf("Sub overflow encountered in function '%s'\n", metadata);
  abort();
}

@pub @pure[stchk no_mangle] func __WDH_mul_overflow(metadata: string) : void {
  puts("*** [ mul overflow ] ***");
  printf("Mul overflow encountered in function '%s'\n", metadata);
  abort();
}

@pub @pure[stchk no_mangle] func __WDH_div_overflow(metadata: string) : void {
  puts("*** [ div overflow ] ***");
  printf("Div overflow encountered in function '%s'\n", metadata);
  abort();
}

@pub @pure[stchk no_mangle] func __WDH_out_of_bounds(metadata: string) : void {
  puts("*** [ out of bounds ] ***");
  printf("Array indexing out of bounds encountered in function '%s'\n", metadata);
  abort();
}

@pub @pure[stchk no_mangle] func __WDH_guard_failed(metadata: string): void {
  puts("*** [ ptr guard failed ] ***");
  printf("Pointer guard failed in function '%s'\n", metadata);
  abort();
}

@pub @pure[stchk no_mangle] func __WD_canary_fail(metadata: string) : void {
  puts("*** [ canary fail ] ***");
  printf("Canary fail encountered in function '%s'\n", metadata);
  abort();
}