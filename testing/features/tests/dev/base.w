@include[
    "#libc.w"
]

func main(): int {
    var buff: [char;32];
    __builtin_memset(buff, 0, 32);
    
    var user_i: int32;
    printf("name: ");
    scanf("%s", buff);
    printf("index: ");
    scanf("%d", &user_i);
    var user_c: char = buff[user_i];
    var square: int32 = user_i * user_i;
    printf("buff[%d] = %c ; Squared i = %d\n", user_i, user_c, square);
    return 0;
}