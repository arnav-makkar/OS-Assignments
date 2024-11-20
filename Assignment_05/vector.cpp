#include "simple-multithreader.h"
#include <assert.h>
#include <pthread.h>

typedef struct {
    int start;
    int end;
    std::function<void(int)> func;
} ThreadArg;

void* thread_func(void* arg) {
    ThreadArg* threadArg = static_cast<ThreadArg*>(arg);

    for (int i = threadArg->start; i < threadArg->end; i++) {
        threadArg->func(i);
    }

    return NULL;
}

void parallel_for(int start, int end, std::function<void(int)> func, int numThread) {
    int totalSize = end - start;
    int chunkSize = (totalSize + numThread - 1) / numThread;

    std::vector<pthread_t> threads(numThread);
    std::vector<ThreadArg> args(numThread);

    for (int i = 0; i < numThread; ++i) {
        args[i].start = start + i * chunkSize;
        args[i].end = std::min(start + (i + 1) * chunkSize, end);
        args[i].func = func;
        pthread_create(&threads[i], nullptr, thread_func, &args[i]);
    }

    for (int i = 0; i < numThread; ++i) {
        pthread_join(threads[i], nullptr);
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

    // Start the parallel addition of two vectors
    parallel_for(0, size, [&](int i) {
        C[i] = A[i] + B[i];
    }, numThread);

    // Verify the result vector
    for (int i = 0; i < size; i++) assert(C[i] == 2);

    printf("Test Success\n");

    // Cleanup memory
    delete[] A;
    delete[] B;
    delete[] C;

    return 0;
}