@extern func _Exit(code: int) : void;
@extern func main() : int;

@pure[stack logue stchk] func __WDcanary_gen() : void {
  asm {
    rdtsc;
    mov qword ptr fs:[0x40], rdi;
  }
}

@pub @pure[stack logue stchk] func _start() : void {
  __WDcanary_gen();
  _Exit(main());
}