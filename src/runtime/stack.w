@include [
    "#types.w"
]

@extern func puts(str: string) : void;
@extern func printf(fmt: string, ...) : void;
@extern func abort() : void;

@pub @pure[stchk] func __WD_canary_fail(ptr: long, name: string) : void {
  puts("[STACK CHECK] *** Integrity check failed ***");
  printf("** BACKTRACE: %s (%p) **\n", name, ptr);
  abort();
}