@include[
    "#libc.wi"
]

namespace testsp {
    func test1(): int {
        printf("Test 1\n");
        return 5;
    }
}

func main(argc: int, argv: ptr<ptr<char>>): int {
    testsp::test1();
    return 0;
}