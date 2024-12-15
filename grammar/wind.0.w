@extern func memcpy(dest: long, src: long, size: int) : long;
@extern func printf(str: long, ...) : int;

//@type ptr = long;
//@type string = ptr;

func main() : int {
	var localVar: [byte; 32];
	var strVar: long = "Hai";
	strVar = strVar + 1;
	memcpy(localVar, "Hello", 5);
	printf("Mem: %s\n" localVar);
	printf("Int: %s\n" strVar);
	return 0;
}
