@include "types.w"

@extern func memcpy(dest: ptr, src: ptr, size: int) : ptr;
@extern func printf(str: string, ...) : int;
@extern func puts(str: string) : int;
@extern func scanf(str: string, ...) : int;