@include[
    "#libc.w"
]

global gb_i: int = 1;

func main(): int {
    var t2:short = 5;
    gb_i = (t2%2)>1;
    printf("Hi %d\n", gb_i);
    branch [
        t2>2: printf(">2\n");
        else: printf("<=2\n");
    ]
    return 0;
}