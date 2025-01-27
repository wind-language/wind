@import "#std"

func main(argc: int, argv: ptr<ptr<char>>): int {
    std::io::println("Backend rewrote! std works!");
    return 0;
}