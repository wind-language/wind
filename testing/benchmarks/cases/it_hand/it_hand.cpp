#include <stdio.h>
#include <time.h>

int square(int n) {
    return n*n;
}

int main() {
    char name[32];
    int age;
    printf("Enter your name: ");
    scanf("%s", &name);
    printf("Enter your age: ");
    scanf("%d", &age);
    int square_age = square(age);
    printf("Hello %s, your age is %d and the square of your age is %d\n", name, age, square_age);
    while (square_age>0) {
        square_age = square_age - age;
    }
    return 0;
}