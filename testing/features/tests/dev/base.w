@import "#std"
@include "#libc.wi"

func reverse(s: ptr<char>): void {

}

func set(s: ptr<char>, val: byte, len: size_t): void {
    var i: size_t = 0;
    loop[i<len] {
        s[i] = val;
        i++;
    }
}

func main(argc: int, argv: ptr<ptr<char>>): int {
    var s: ptr<char> = std::mem::alloc(5);
    set(s, 'a', 5);
    printf("%s\n", s);
    std::mem::reverse(s, 5);
    printf("%s\n", s);
    return 0;
}