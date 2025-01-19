@include[
    "#libc.wi"
]
//@import "#link"

func main(argc: int, argv: ptr<ptr<char>>): int {
    var T: s16=30_000;
    
    try {
        T += 20_000; // sum overflow
        T = guard![Null];
    }
    [SUM_OF] -> {
        printf("Sum overflow handled\n");
    }
    [GUARD] -> {
        printf("Null pointer exception handled\n");
    }

    return 0;
}