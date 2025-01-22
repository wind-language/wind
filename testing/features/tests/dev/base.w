@import "#std"

func main(argc: int, argv: ptr<ptr<char>>): int {
    var s: ptr<char> = "Hello";
    std::io::puts(s);
    return 0;
}