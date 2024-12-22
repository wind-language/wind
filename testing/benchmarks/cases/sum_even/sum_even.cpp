#include <stdio.h>

long diff_even(long n) {
    long i=0;
    while (n > 0) {
        n = n-1;
        if (n%2==0) {
            i = n+i;
        } else {
            i = i-n;
        }
        i = i+1;
    }
    return i;
}


int main() {
    long N;
    scanf("%llu", &N);
    long de = diff_even(N);
    printf("dfe(%llu) = %llu\n", N, de);
    return 0;
}