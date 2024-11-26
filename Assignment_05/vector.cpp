#include "simple-multithreader.h"
#include <assert.h>
#include <pthread.h>
#include <time.h>
#include <sys/mman.h>

// Linked list structure to store results
struct ListNode {
    int* data;
    int size;
    ListNode* next;
};

// Utility function to create a new node
ListNode* create_node(int* data, int size) {
    ListNode* node = new ListNode;
    node->data = data;
    node->size = size;
    node->next = nullptr;
    return node;
}

// Function to append a node to the linked list
void append_node(ListNode*& head, ListNode* newNode) {
    if (!head) {
        head = newNode;
    } else {
        ListNode* temp = head;
        while (temp->next) temp = temp->next;
        temp->next = newNode;
    }
}

// Thread argument structure
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

    // Allocate vectors using mmap
    int* A = (int*)mmap(NULL, size * sizeof(int), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    int* B = (int*)mmap(NULL, size * sizeof(int), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    int* C = (int*)mmap(NULL, size * sizeof(int), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (A == MAP_FAILED || B == MAP_FAILED || C == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    // Initialize the vectors
    std::fill(A, A + size, 1);
    std::fill(B, B + size, 1);
    std::fill(C, C + size, 0);

    // Linked list head for results
    ListNode* resultList = nullptr;

    clock_t startTime = clock();

    // Start the parallel addition of two vectors
    parallel_for_1D(0, size, [&](int i) {
        C[i] = A[i] + B[i];
    }, numThread);

    clock_t endTime = clock();
    double execTime = (double)(endTime - startTime) / CLOCKS_PER_SEC;
    printf("Execution Time: %.6f seconds\n", execTime);

    // Append results to linked list
    ListNode* resultNode = create_node(C, size);
    append_node(resultList, resultNode);

    // Verify the result vector
    for (int i = 0; i < size; i++) assert(C[i] == 2);

    printf("Test Success\n");

    // Cleanup memory
    munmap(A, size * sizeof(int));
    munmap(B, size * sizeof(int));
    munmap(C, size * sizeof(int));

    // Cleanup linked list
    while (resultList) {
        ListNode* temp = resultList;
        resultList = resultList->next;
        delete temp;
    }

    return 0;
}