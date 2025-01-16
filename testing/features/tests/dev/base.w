@include[
    "#libc.wi"
]
//@import "#link"

func t7(a: short, b: int, c: int, d:int, e: int, f: int, g: ptr<char>): int {
    printf("g: %p\n", g);
    a += 9000;
    return 0;
}

func main(): int {
    var test_ptr: ptr<char> = guard![malloc(sizeof<char>*32)];
    t7(30000,2,3,4,5,6,test_ptr);
    return 0;
}