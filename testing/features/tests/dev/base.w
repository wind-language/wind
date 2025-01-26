@include "#libc.wi"
@import "#std"


func main(argc: int, argv: ptr<ptr<char>>): int {
    var a: u32 = 10;
    var b: u32 = 20;
    //TODO: fix signess issue
    std::io::println("a: ", a as s32, " b: ", b as s32);
    return 0;
}