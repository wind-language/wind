@include[
    "#libc.w"
]

func main(): int {
    var i:int=0;
    loop [i<10] {
        printf("Hello %d\n", i);
        i++;
    }
    return 0;
}