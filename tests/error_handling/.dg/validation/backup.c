#include <stdio.h>

#define N 10
int arr[N];

int access(){
    arr[N+2] = 10;
    for(int i=0; i<=N; i++)
        printf("%d\n", arr[N]);
}

int main(){
    while (1)
    {
        access();
    }
    
    
}