#include <stdio.h>

#define N 10
int arr[N];

int _access(int i){
    if (ANGELIX_CHOOSE(bool, 1, 7, 5, 7, 26, ((char*[]){"i"}), ((int[]){i}), 1)) printf("%d\n", arr[i]);
    else return 1;
	return 0;
}

int main(){
    
    return _access(10);
    
}
