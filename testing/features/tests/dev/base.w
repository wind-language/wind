@include[
    "#libc.w"
]
/* @import [
    "#link"
] */

func get_ptr(): ptr<ptr<int>> {
    var buff: ptr<ptr<int>> = malloc(8*4);
    buff[i] = malloc(4*4);
    buff[i][0] = 1;
    return buff;
}

func main(): int {
    var buff: ptr<int> = get_ptr();
    var i: int = 5;
    printf("buff[i] = %d\n", buff[i]);
    return 0;
}