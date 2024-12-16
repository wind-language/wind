@include "libc.w"
@include "types.w"

func main() : int {
	var localVar: [byte; 32];
	memcpy(localVar, "Hello", 5);
	localVar[1] = 'J';
	printf("Mem: %s\n" localVar);
	add(1);
	return 0;
}
