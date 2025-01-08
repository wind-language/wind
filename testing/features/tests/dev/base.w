@include[
    "#libc.w"
]

func main(): int {
    var i:int16=0;
    loop [i<10] {
        printf("Hello %hd\n", i);
        i++;
    }
    i = 32767;
    i = i+1;
    printf("Res: %hu\n", i);
    return 0;
}