@include [
  "#libc.w"
]

global maxRes:long=-1;
func occrec(N: int, last: long, M: long, i: int): long {
    branch[
        N==i: return 0;
    ]
    var maxRes: long=-1;
    var digit: int=3;
    loop[digit<=9] {
        branch[
            digit == (last%10): {}
            else : {
                var newLast: long = (last*10)+digit;
                var modc: long = newLast%M;
                branch [
                    modc > maxRes: {
                        maxRes = modc;
                    }
                ]
                var modr: long = occrec(N, newLast, M, i+1);
                branch[
                    modr > maxRes: {
                        maxRes = modr;
                    }
                ]
            }
        ]
        digit=digit+3;
    }
    return maxRes;
}

func occulta(N: int, M: int): long {
    return occrec(N,0,M,maxRes);
}

func main(): int {
    var N: int;
    var M: int;
    var T: int;
    var i: int = 0;
    scanf("%d", &T);
    loop[i<T] {
        scanf("%d %d", &N, &M);
        var max: long = occulta(N,M);
        printf("%ld\n", max);
        i=i+1;
    }
    return 0;
}