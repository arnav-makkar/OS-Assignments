#include "simple-multithreader.h"
#include <assert.h>
#include <pthread.h>
#include <time.h>

struct ThreadArg {
	int start;
	int end;
	int cols;
	std::function<void(int)> func1;
    std::function<void(int, int)> func2;
};

void* thread_func1(void* arg) {
    ThreadArg* threadArg = (ThreadArg*)(arg);
    for (int i = threadArg->start; i < threadArg->end; i++) {
        threadArg->func1(i);
    }
    return NULL;
}

void* thread_func2(void* arg) {
    ThreadArg* threadArg = (ThreadArg*)(arg);
    for (int i = threadArg->start; i <= threadArg->end; i++) {
        threadArg->func2(i/threadArg->cols, i%threadArg->cols);
    }
    return NULL;
}

void parallel_for_2D(int s1, int e1, int s2, int e2, std::function<void(int, int)> func, int numThread) {
    int rows = e1 - s1;
    int cols = e2 - s2;
    int totalSize = rows * cols;
    int chunkSize = (totalSize + numThread - 1) / numThread;

    pthread_t threads[numThread];
    ThreadArg args[numThread];

    for (int i = 0; i < numThread; ++i) {
        int start = i * chunkSize;
        int end = std::min(start + chunkSize-1, totalSize-1);

		args[i].start = start;
		args[i].end = end;
		args[i].cols = cols;

        args[i].func2 = func;
        pthread_create(&threads[i], NULL, thread_func2, &args[i]);
    }

    for (int i = 0; i < numThread; ++i) {
        pthread_join(threads[i], NULL);
    }
}

void parallel_for_1D(int start, int end, std::function<void(int)> func, int numThread) {
    int totalSize = end - start;
    int chunkSize = (totalSize + numThread - 1) / numThread;

    pthread_t threads[numThread];
    ThreadArg args[numThread];

    for (int i = 0; i < numThread; ++i) {
        args[i].start = start + i * chunkSize;
        args[i].end = std::min(start + (i + 1) * chunkSize, end);
        args[i].func1 = func;
        pthread_create(&threads[i], NULL, thread_func1, &args[i]);
    }

    for (int i = 0; i < numThread; ++i) {
        pthread_join(threads[i], NULL);
    }
}

int main(int argc, char** argv) {
    // Initialize problem size
    int numThread = argc > 1 ? atoi(argv[1]) : 2;
    int size = argc > 2 ? atoi(argv[2]) : 1024;

    // Allocate matrices
    int** A = new int*[size];
    int** B = new int*[size];
    int** C = new int*[size];
    parallel_for_1D(0, size, [&](int i) {
        A[i] = new int[size];
        B[i] = new int[size];
        C[i] = new int[size];
        std::fill(A[i], A[i] + size, 1);
        std::fill(B[i], B[i] + size, 1);
        std::fill(C[i], C[i] + size, 0);
    }, numThread);

	clock_t startTime = clock();

    // Start the parallel multiplication of two matrices
    parallel_for_2D(0, size, 0, size, [&](int i, int j) {
        for (int k = 0; k < size; k++) {
            C[i][j] += A[i][k] * B[k][j];
        }
    }, numThread);

	clock_t endTime = clock();
    double execTime = (double)(endTime - startTime) / CLOCKS_PER_SEC;
    printf("Execution Time: %.6f seconds\n", execTime);

    // Verify the result matrix
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            assert(C[i][j] == size);
        }
    }
    printf("Test Success.\n");

    // Cleanup memory
    parallel_for_1D(0, size, [&](int i) {
        delete[] A[i];
        delete[] B[i];
        delete[] C[i];
    }, numThread);
    delete[] A;
    delete[] B;
    delete[] C;

    return 0;
}