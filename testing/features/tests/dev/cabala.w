@include [
  "#libc.w"
]

//var maxRes: long=-1;
func occrec(N: int, last: long, M: long, i: int): void {
    branch[
        N==i: return;
    ]
    var digit: int=3;
    loop[digit<=9] {
        branch[
            digit == (last%10): {}
            else : {
                var newLast: long = (last*10)+digit;
                var modc: long = newLast%M;
                printf("ModC: %ld\n", modc);
                occrec(N, newLast, M, i+1);
            }
        ]
        digit=digit+3;
    }
}

func occulta(N: int, M: int): long {
    //maxRes=-1;
    occrec(N,0,M,0);
    //return maxRes;
    return 0;
}

func main(): int {
    var N: int;
    var M: int;
    scanf("%d %d", &N, &M);
    occulta(N,M);
    return 0;
}