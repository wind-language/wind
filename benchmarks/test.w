@extern func memcpy(dest: long, src: long, size: int) : long;
@extern func printf(str: long, ...) : int;

func main() : int {
	var localVar: [byte; 32];
	memcpy(localVar, "Hello", 5);
	printf("Mem: %s\n" localVar);
	return 0;
}
