@include "types.w"

@extern func memcpy(dest: ptr, src: ptr, size: int) : ptr;
@extern func printf(str: string, ...) : int;
@extern func puts(str: string) : int;
@extern func scanf(str: string, ...) : int;
@extern func memset(dest: ptr, c: int, size: int) : ptr;
@extern func malloc(size: int) : ptr;
@extern func free(ptr: ptr) : void;
@extern func realloc(ptr: ptr, size: int) : ptr;
@extern func strlen(str: string) : int;
@extern func strncat(dest: string, src: string, n: int) : string;


// Wind runtime functions useful for the user
@extern func __builtin_copy(dst: ptr, src: ptr, size: uint64): void; 
@extern func __builtin_memset(dst: ptr, c: char, size: uint64): void;