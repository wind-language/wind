@include [
  "#libc.w"
]

func diff_even(n: int): int {
  var i: int=0;
  loop [n > 0] {
    n = n-1;
    i = i+1;
  }
  return i;
}

func main(): int {
  var N: int;
  scanf("%d", &N);
  var de: long = diff_even(N);
  printf("dfe(%d) = %ld\n", N, de);
}