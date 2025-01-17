@include [
  "#libc.wi"
]

global maxRes:int64=-1;
func occrec(N: int, last: int64, M: int64, i: int): void {
    branch[
        N==i: return;
    ]
    var digit: int=3;
    loop[digit<=9] {
        branch[
            digit == (last%10): {}
            else : {
                var newLast: int64 = (last*10)+digit;
                var modc: int64 = newLast%M;
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

func occulta(N: int, M: int): int64 {
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