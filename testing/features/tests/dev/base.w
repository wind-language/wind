@include[
    "#libc.w"
]

global test: int = 1;

func main(): int {
    var t2:int = 5;
    test = t2%2;
    printf("Hi %d\n", test);
    return 0;
}