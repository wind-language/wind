@extern @pure[no_mangle] func _Exit(code: int) : void;
@extern @pure[no_mangle] func main() : int;

@pub @pure[stack logue stchk no_mangle] func _start() : void {
  asm {
    mov rdi, [rsp];
    lea rsi, [rsp + 8];
  }
  _Exit(main());
}