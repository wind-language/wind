@include[
    "#libc.wi"
]

func main(argc: int, argv: ptr<ptr<char>>): int {
    var T: s16=30_000;
    var i: s16=0;

    loop [true] {
        printf("Enter a number to sum to %d within 16 bits: ", T);
        scanf("%hu", &i);
        try {
            T += i;
        }
        [SUM_OF] -> {
            printf("Sum overflowed 16 bits\n");
        }
        finally {
            continue;
        }
        break;
    }
    printf("Sum: %d\n", T);

    return 0;
}