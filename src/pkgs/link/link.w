@include [
    "#libc.wi"
]
@import [
    "#curl"
]

@pub func test1(): void {
    curl_test();
    puts("Test worked!");
}