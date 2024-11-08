#include "loader.h"
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <elf.h>

// ELF header and program header pointers
Elf32_Ehdr *ehdr = NULL;
Elf32_Phdr *phdr = NULL;
int fd = -1;
void **segments = NULL; // Array to store mmap'ed segment pointers

// Statistics for page faults and memory allocations
size_t page_fault_count = 0; 
size_t page_alloc_count = 0;
size_t total_fragmentation = 0;

// Page size constant (4KB)
#define PAGE_SIZE 4096

// Release memory and other cleanups
void loader_cleanup() {
    if (fd >= 0) close(fd);
    if (ehdr) free(ehdr);
    if (phdr) free(phdr);
    if (segments) free(segments);
}

// Function to calculate internal fragmentation for a segment
size_t calculate_fragmentation(void *segment_start, size_t segment_size) {
    size_t allocated_pages = (segment_size + PAGE_SIZE - 1) / PAGE_SIZE;
    size_t total_allocated_size = allocated_pages * PAGE_SIZE;
    return total_allocated_size - segment_size;
}

// Signal handler for segmentation fault (page fault)
void sigsegv_handler(int sig, siginfo_t *si, void *unused) {
    void *fault_addr = si->si_addr;
    void *seg_start = NULL;
    void *seg_end = NULL;
    size_t segment_offset = 0;
    int segment_index = -1;

    // Find the segment that contains the fault address
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            seg_start = (void *)phdr[i].p_vaddr;
            seg_end = (void *)(phdr[i].p_vaddr + phdr[i].p_memsz);
            if (fault_addr >= seg_start && fault_addr < seg_end) {
                segment_index = i;
                segment_offset = (size_t)fault_addr - (size_t)seg_start;
                break;
            }
        }
    }

    if (segment_index == -1) {
        // Should not happen, just return if the fault doesn't belong to any segment
        return;
    }

    // Calculate the page offset and which page to allocate
    size_t page_offset = segment_offset & ~(PAGE_SIZE - 1);
    void *page_addr = (void *)((size_t)seg_start + page_offset);

    // Map the page lazily when accessed
    void *mapped_page = mmap(page_addr, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC,
                              MAP_PRIVATE, fd, phdr[segment_index].p_offset + page_offset);
    if (mapped_page == MAP_FAILED) {
        perror("Failed to map additional page");
        exit(1);
    }

    // Increment the page fault count and allocation count
    page_fault_count++;
    page_alloc_count++;

    // If we're loading a segment, calculate fragmentation
    size_t fragment = calculate_fragmentation(seg_start, phdr[segment_index].p_memsz);
    total_fragmentation += fragment;

    printf("Page fault at address %p, mapped page at %p\n", fault_addr, mapped_page);
}

// Function to load and run the ELF executable file
void load_and_run_elf(const char **exe) {
    fd = open(exe[1], O_RDONLY);
    if (fd < 0) {
        perror("Failed to open the ELF file\n");
        loader_cleanup();
        exit(1);
    }

    // 1. Load the ELF header
    ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
    if (read(fd, ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) {
        perror("Failed to read ELF header\n");
        loader_cleanup();
        exit(1);
    }

    off_t e_phoff = ehdr->e_phoff;
    uint16_t e_phnum = ehdr->e_phnum;

    // 2. Allocate memory for program headers and read them
    phdr = (Elf32_Phdr *)malloc(e_phnum * sizeof(Elf32_Phdr));
    lseek(fd, e_phoff, SEEK_SET);
    if (read(fd, phdr, e_phnum * sizeof(Elf32_Phdr)) != (e_phnum * sizeof(Elf32_Phdr))) {
        perror("Failed to read program headers\n");
        loader_cleanup();
        exit(1);
    }

    // 3. Setup signal handler for page faults (SIGSEGV)
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = sigsegv_handler;
    sigaction(SIGSEGV, &sa, NULL);

    // 4. Load segments lazily
    segments = calloc(e_phnum, sizeof(void *)); // Allocate memory for storing segment addresses
    for (int i = 0; i < e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            // Initially map the first page of the segment (this is the crucial part)
            void *first_page = mmap((void *)phdr[i].p_vaddr, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC,
                                    MAP_PRIVATE, fd, phdr[i].p_offset);
            if (first_page == MAP_FAILED) {
                perror("Initial mmap failed");
                loader_cleanup();
                exit(1);
            }
            segments[i] = first_page; // Store the segment base address
            printf("Segment %d mapped at address %p\n", i, first_page);
        }
    }

    // 5. Find and execute the entry point
    for (int i = 0; i < e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            // Check if entry point is within this segment
            if (ehdr->e_entry >= phdr[i].p_vaddr && ehdr->e_entry < phdr[i].p_vaddr + phdr[i].p_memsz) {
                void *entry_point = (void *)(phdr[i].p_vaddr + (ehdr->e_entry - phdr[i].p_vaddr));
                printf("Jumping to entry point at address %p\n", entry_point);

                // Cast the entry point to a function pointer and call it
                int (*_start)() = (int (*)())entry_point;
                int result = _start();  // Start the program
                printf("User _start return value = %d\n", result);
                break;
            }
        }
    }

    // 6. Report statistics after execution
    printf("\nProgram execution completed.\n");
    printf("Total page faults: %zu\n", page_fault_count);
    printf("Total page allocations: %zu\n", page_alloc_count);
    printf("Total internal fragmentation: %zu KB\n", total_fragmentation / 1024);

    loader_cleanup();
}

int main(int argc, const char **argv) {
    if (argc != 2) {
        printf("Usage: %s <ELF Executable>\n", argv[0]);
        exit(1);
    }
    load_and_run_elf(argv);
    return 0;
}
