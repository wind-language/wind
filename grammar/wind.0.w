@extern func malloc(size: int) : long;
@extern func memset(ptr: long, value: byte, size: int) : long;
@extern func puts(str: long) : int;
@extern func printf(str: long, ...) : int;

func add(a: int, b: int) : short {
	return a + b;
}

func main() : int {
	var localVar: int = 0;
	memset(&localVar, 1, 16);
	printf("Sum: %d\n" add(1,2));
	return localVar;
}
