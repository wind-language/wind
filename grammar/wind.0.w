@extern func malloc(size: int) : long;
@extern func puts(str: long) : int;
@extern func printf(str: long, ...) : int;

func add(a: int, b: int) : short {
	return a + b;
}

func main() : int {
	var somma: int = add(1,2);
	printf("Somma: %d\n", somma);
	return 0;
}
