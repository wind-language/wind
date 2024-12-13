@extern func malloc(size: int) : long;
@extern func puts(str: long) : int;
@extern func printf(str: long, ...) : int;

func add(a: int, b: int) : short {
	return a + b;
}

func main() : int {
	var m: long = malloc(64);
	asm {
		mov rax, ?m;
		mov byte ptr [rax], 0x48;
		mov byte ptr [rax+1], 0x65;
	}
	printf("Sum: %s\n" m);
	return 0;
}
