
func testAsm(arg: int) : void {
	asm {
		mov eax, 0x1234;
		mov ebx, ?arg;
		add eax, ebx;
	}
}

func add(a: int, b: int) : short {
	return a + b;
}

func main() : int {
	var sum: int = add(1, 2)+5;
	return sum;
}
