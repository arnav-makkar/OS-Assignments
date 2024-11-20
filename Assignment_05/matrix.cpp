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
	// intialize problem size
	int numThread = argc>1 ? atoi(argv[1]) : 2;
	int size = argc>2 ? atoi(argv[2]) : 1024;  
	// allocate matrices
	int** A = new int*[size];
	int** B = new int*[size];
	int** C = new int*[size];
	parallel_for(0, size, [=](int i) {
	A[i] = new int[size];
	B[i] = new int[size];
	C[i] = new int[size];
	for(int j=0; j<size; j++) {
		// initialize the matrices
		std::fill(A[i], A[i]+size, 1);
		std::fill(B[i], B[i]+size, 1);
		std::fill(C[i], C[i]+size, 0);
	}
	}, numThread);  
	// start the parallel multiplication of two matrices
	parallel_for(0, size, 0, size, [&](int i, int j) {
	for(int k=0; k<size; k++) {
		C[i][j] += A[i][k] * B[k][j];
	}
	}, numThread);
	// verify the result matrix
	for(int i=0; i<size; i++) for(int j=0; j<size; j++) assert(C[i][j] == size);
	printf("Test Success. \n");
	// cleanup memory
	parallel_for(0, size, [=](int i) {
	delete [] A[i];
	delete [] B[i];
	delete [] C[i];
	}, numThread);
	delete[] A;
	delete[] B;
	delete[] C;
	return 0;
}
