#include <stdio.h>

long long occrec(int N, long long last, long long M, int i) {
    if (N==i) return 0;
    long long maxRes=-1;
    for(int digit=3;digit<=9;digit+=3) {
        if (digit == (last%10)) {
        } else {
            long long newLast = (last*10)+digit;
            long long modc = newLast%M;
            if (modc > maxRes) {
                maxRes = modc;
            }
            long long modr = occrec(N, newLast, M, i+1);
            if (modr >maxRes) {
                maxRes = modr;
            }
        }
    }
    return maxRes;
}

long long occulta(int N, int M) {
    return occrec(N,0,M,0);
}

int main() {
    int N,M,T;
    scanf("%d",&T);
    for (int i=0;i<T;i++) {
        scanf("%d %d",&N,&M);
        long long max = occulta(N,M);
        printf("%lld\n",max);
    }
}