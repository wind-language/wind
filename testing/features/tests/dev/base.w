@include[
    "#libc.w"
]

func main(): int {
    var buff: [char;32];
    __builtin_memset(buff, 0, 32);
    __builtin_copy(buff, "Hello World", 12);
    var i: int=0;
    loop [i<32] {
        printf("buff[%d]: %c\n", i, buff[i]);
        i++;
    }
    return 0;
}