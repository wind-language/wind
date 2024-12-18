@include [
  "#libc.w"
]

func sum(n: long) : long {
  // this is going to be optimized into an inlined function
  return n * (n + 1) / 2;
}

func main() : int {
  printf("Sum: %llu\n", sum(50));
  return 0;
}
