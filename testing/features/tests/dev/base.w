@include[
    "#libc.w"
]

func main(): int {
    var buff: [char;32];
    var i: int=5;
    buff[5] = 'a';
    printf("buff[i]: %c\n", buff[i]);
    buff[40] = 'b';
    return 0;
}