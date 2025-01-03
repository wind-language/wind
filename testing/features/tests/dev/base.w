@include[
    "#libc.w"
]

global gb_i: int = 1;

func main(): int {
    var t2:int = 5;
    gb_i = (t2%2)>1;
    printf("Hi %d\n", gb_i);
    loop [gb_i < t2] {
        printf("Hello %d\n", gb_i);
        gb_i = gb_i + 1;
    }
    return 0;
}