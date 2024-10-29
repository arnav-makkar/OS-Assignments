#include <stdio.h>
#include "dummy_main.h"
#include <stdlib.h>

int fib(int n) {
    if(n<2) return n;

    else{
        return fib(n-1) + fib(n-2);
    }
}

int main(int argc, char **argv){

    if (argc != 2) {
        printf("Usage: %s <number>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    int result = fib(n);
    printf("Fibonacci of %d is %d\n", n, result);

    return 0;
}
