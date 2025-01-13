@include "types.w"

@extern func memcpy(dest: uint64, src: uint64, size: int) : uint64;
@extern func printf(str: string, ...) : int;
@extern func puts(str: string) : int;
@extern func scanf(str: string, ...) : int;
@extern func memset(dest: uint64, c: int, size: int) : uint64;
@extern func malloc(size: int) : uint64;
@extern func free(ptr: uint64) : void;
@extern func realloc(ptr: uint64, size: int) : uint64;
@extern func strlen(str: string) : int;
@extern func strncat(dest: string, src: string, n: int) : string;


// Wind runtime functions useful for the user
@extern func __builtin_copy(dst: uint64, src: uint64, size: uint64): void; 
@extern func __builtin_memset(dst: uint64, c: char, size: uint64): void;