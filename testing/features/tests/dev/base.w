@include[
    "#libc.w"
]
/* @import [
    "#link"
] */

func get_ptr(): ptr<char> {
    var buff: ptr<char> = malloc(10);
    buff[1] = 'a';
    return buff;
}

func main(): int {
    var buff: ptr<char> = get_ptr();
    printf("buff[1] = %c\n", buff[1]);
    return 0;
}