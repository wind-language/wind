@include[
    "#libc.w"
]
//@import "#link"

func main(): int {
    var u64ptr: ptr<uint64> = malloc(8*16);
    u64ptr[0] = malloc(32);
    (cast< ptr<char> >(u64ptr[0]))[0] = 'H';
    printf("%p\n", u64ptr[0]);
    printf("%s\n", u64ptr[0]);
    return 0;
}