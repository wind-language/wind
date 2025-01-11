@include "types.w"

@extern func memcpy(dest: ptr, src: ptr, size: int) : ptr;
@extern func printf(str: string, ...) : int;
@extern func puts(str: string) : int;
@extern func scanf(str: string, ...) : int;
@extern func memset(dest: ptr, c: int, size: int) : ptr;


// Wind runtime functions useful for the user
@extern func __builtin_copy(dst: ptr, src: ptr, size: uint64): void; 
@extern func __builtin_memset(dst: ptr, c: char, size: uint64): void;