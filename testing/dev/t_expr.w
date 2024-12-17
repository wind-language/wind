@include "../../src/std/libc.w"

func sum(n: long) : long {
  return n * (n + 1) / 2;
}

func main() : int {
	printf("Sum: %llu\n", sum(5000000));
	return 0;
}
