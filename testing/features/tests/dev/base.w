@include[
    "#libc.wi"
]
//@import "#link"

func main(): int {
    var test_ptr: ptr<ptr<char>> = guard![malloc(sizeof<ptr<char>>*32)];
    test_ptr[test_ptr[0]] = "Hello";
    printf("test_ptr[0]: %s\n", test_ptr[0]);
    return 0;
}