@include[
    "#libc.wi"
]
//@import "#link"

func main(argc: int, argv: ptr<ptr<char>>): int {
    var T: s16=30_000;
    try {
        T += 20_000; // sum overflow
    } [SUM_OF] -> {
        printf("Sum overflow handled\n");
    }
    return 0;
}