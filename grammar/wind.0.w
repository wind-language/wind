@extern func malloc(size: int) : long;
@extern func puts(str: long) : int;

func add(a: int, b: int) : short {
	return a + b;
}

func main() : int {
	var sum: int = add(1, 2);
	var addr: long = malloc(64);
	puts("Hello");
	return addr+sum;
}
