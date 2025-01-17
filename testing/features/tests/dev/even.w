@include [
  "#libc.wi"
]

func main(): int {
  var N: int;
  var max_sum: int = -1;

  scanf("%d", &N);
  var i: int = 0;

  loop [i < N] {
    var a: int;
    var b: int;
    scanf("%d %d", &a, &b);
    var sum: int = a + b;
    branch [
      (sum % 2 == 0 && max_sum < sum) : {
        max_sum = sum;
      }
    ]
    i = i+1;
  }
  printf("%d\n", max_sum);
  return 0;
}