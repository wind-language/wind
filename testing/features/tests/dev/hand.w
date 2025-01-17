@include[
    "#libc.wi"
]

func square(n: int): int {
    return n*n;
}

func main(): int {
    var name: [char;32];
    var age: int;
    printf("Enter your name: ");
    scanf("%s", &name);
    printf("Enter your age: ");
    scanf("%d", &age);
    var square_age: int = square(age);
    printf("Hello %s, your age is %d and the square of your age is %d\n", name, age, square_age);
    loop [square_age>0] {
        square_age = square_age - age;
    }
    return 0;
}