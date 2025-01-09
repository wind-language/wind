#include <stdio.h>

long long maxRes=-1;
void occrec(int N, long long last, long long M, int i) {
    if (N==i) return;
    for(int digit=3;digit<=9;digit+=3) {
        if (digit == (last%10)) {
        } else {
            long long newLast = (last*10)+digit;
            long long modc = newLast%M;
            if (modc > maxRes) {
                maxRes = modc;
            }
            occrec(N, newLast, M, i+1);
        }
    }
}

long long occulta(int N, int M) {
    maxRes=-1;
    occrec(N,0,M,0);
    return maxRes;
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