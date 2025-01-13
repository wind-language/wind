@include[
    "#libc.w"
]
/* @import [
    "#link"
] */

func get_ptr(): ptr<int> {
    // deliberately allocate a large buffer to make malloc fail
    var buff: ptr<int> = guard![malloc(32*900000000)];
    return buff;
}

func main(): int {
    var buff: ptr<int> = get_ptr();
    printf("Buff: %p\n", buff);
    return 0;
}