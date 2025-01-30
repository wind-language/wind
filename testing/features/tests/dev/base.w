@import "#std"


struct FooBar {
    e: int
}
struct Bar {
    c: int,
    d: FooBar
}
struct Foo {
    a: int,
    b: int,
    c: Bar,
}

func main(argc: int, argv: ptr<ptr<char>>): int {
    var foo: Foo = {
        a: 1,
        b: 2,
        c: {c: 3, d: {e: 4}},
    };
    std::io::println("foo.c.d.e : ", foo.c.d.e);
    return 0;
}
