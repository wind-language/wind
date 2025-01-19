@include [
  "#libc.wi"
]

global maxRes:s64=-1;
func occrec(N: int, last: s64, M: s64, i: int): void {
    branch[
        N==i: return;
    ]
    var digit: int=3;
    loop[digit<=9] {
        branch[
            digit == (last%10): {}
            else : {
                var newLast: s64 = (last*10)+digit;
                var modc: s64 = newLast%M;
                branch [
                    modc > maxRes: {
                        maxRes = modc;
                    }
                ]
                occrec(N, newLast, M, i+1);
            }
        ]
        digit+=3;
    }
}

func occulta(N: int, M: int): s64 {
    maxRes=-1;
    occrec(N,0,M,-1);
    return maxRes;
}

func main(): int {
    var [N,M,T]: int;
    var i: int = 0;
    scanf("%d", &T);
    loop[i<T] {
        scanf("%d %d", &N, &M);
        printf("%ld\n", occulta(N,M));
        i++;
    }
    return 0;
}