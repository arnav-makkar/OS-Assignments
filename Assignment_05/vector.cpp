#include "simple-multithreader.h"
#include <assert.h>
#include <pthread.h>
#include <time.h>

typedef struct {
    int start;
    int end;
    std::function<void(int)> func;
} ThreadArg;

void* thread_func(void* arg) {
    ThreadArg* threadArg = (ThreadArg*)(arg);

    for (int i = threadArg->start; i < threadArg->end; i++) {
        threadArg->func(i);
    }

    return NULL;
}

void parallel_for_1D(int start, int end, std::function<void(int)> func, int numThread) {
    int totalSize = end - start;
    int chunkSize = (totalSize + numThread - 1) / numThread;

    pthread_t threads[numThread];
    ThreadArg args[numThread];

    for (int i = 0; i < numThread; ++i) {
        args[i].start = start + i * chunkSize;
        args[i].end = std::min(start + (i + 1) * chunkSize, end);
        args[i].func = func;
        pthread_create(&threads[i], NULL, thread_func, &args[i]);
    }

    for (int i = 0; i < numThread; ++i) {
        pthread_join(threads[i], NULL);
    }
}

int main(int argc, char** argv) {
    // Initialize problem size
    int numThread = argc > 1 ? atoi(argv[1]) : 2;
    int size = argc > 2 ? atoi(argv[2]) : 48000000;

    // Allocate vectors
    int* A = new int[size];
    int* B = new int[size];
    int* C = new int[size];

    // Initialize the vectors
    std::fill(A, A + size, 1);
    std::fill(B, B + size, 1);
    std::fill(C, C + size, 0);

    clock_t startTime = clock();

    // Start the parallel addition of two vectors
    parallel_for_1D(0, size, [&](int i) {
        C[i] = A[i] + B[i];
    }, numThread);

    clock_t endTime = clock();
    double execTime = (double)(endTime - startTime) / CLOCKS_PER_SEC;
    printf("Execution Time: %.6f seconds\n", execTime);

    // Verify the result vector
    for (int i = 0; i < size; i++) assert(C[i] == 2);

    printf("Test Success\n");

    // Cleanup memory
    delete[] A;
    delete[] B;
    delete[] C;

    return 0;
}