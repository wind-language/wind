@include[
    "#libc.wi"
]

func main(argc: int, argv: ptr<ptr<char>>): int {
    var T: s16=30_000;
    var i: int=0;
    var tptr: ptr<int> = guard![malloc(sizeof<int>*8)];

    loop [true] {
        printf("Enter a number to sum to %hd within 16 bits: ", T);
        scanf("%hd", &i);
        try {
            T += (i :: s16) + (tptr[0] :: s16);
        }
        [SUM_OF] -> {
            printf("Sum overflowed 16 bits\n");
        }
        finally {
            continue;
        }
        break;
    }
    printf("Sum: %hd\n", T);

    return 0;
}