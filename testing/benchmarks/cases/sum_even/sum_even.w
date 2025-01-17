@include [
  "#libc.wi"
]

func diff_even(n: long): long {
  var i: long=0;
  loop [n > 0] {
    n = n-1;
    branch[
      n % 2 == 0: i = n+i;
      else : i = i-n;
    ]
    i = i+1;
  }
  return i;
}

func main(): int {
  var N: long;
  scanf("%llu", &N);
  var de: long = diff_even(N);
  printf("dfe(%llu) = %llu\n", N, de);
}