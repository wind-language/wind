@include "libc.w"

func main() : int {
	var localVar: [byte; 32];
	memcpy(localVar, "Hello", 5);
	localVar[1] = 'J';
	printf("Mem: %s\n" localVar);
	return 0;
}
