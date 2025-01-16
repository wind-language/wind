@include[
    "#libc.w"
]
//@import "#link"

func t7(a: int, b: int, c: int, d:int, e: int, f: int, g: ptr<char>): int {
    printf("g: %p\n", g);
}

func main(): int {
    var test_ptr: ptr<char> = malloc(sizeof<char>*32);
    t7(1,2,3,4,5,6,test_ptr);
    return 0;
}