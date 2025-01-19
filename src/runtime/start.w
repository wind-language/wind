@extern func _Exit(code: int) : void;
@extern func main() : int;

@pure[stack logue stchk] func __WD_canary_gen() : void {
  asm {
    rdtsc;
    mov qword ptr fs:[0x40], rdi;
  }
}

@pub @pure[stack logue stchk] func _start() : void {
  __WD_canary_gen();
  asm {
    mov rdi, [rsp];
    lea rsi, [rsp + 8];
  }
  _Exit(main());
}