#include "loader.h"
#include <signal.h>   // Standard signal handling header
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <elf.h>

Elf32_Ehdr *ehdr = NULL;
Elf32_Phdr *phdr = NULL;
int fd = -1;
void **segments = NULL; // Array to store mmap'ed segment pointers
size_t page_fault_count = 0; // Counter for page faults

// Page size constant (4KB)
#define PAGE_SIZE 4096

// Handler for segmentation faults
void segfault_handler(int sig) {
    void *fault_addr = (void*)sig;

    // Locate the segment that caused the fault
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD && fault_addr >= (void *)phdr[i].p_vaddr &&
            fault_addr < (void *)(phdr[i].p_vaddr + phdr[i].p_memsz)) {

            // Calculate the page-aligned address to mmap
            uintptr_t page_addr = ((uintptr_t)fault_addr / PAGE_SIZE) * PAGE_SIZE;
            void *mapped_addr = mmap((void *)page_addr, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC,
                                     MAP_PRIVATE, fd, phdr[i].p_offset + (page_addr - phdr[i].p_vaddr));

            if (mapped_addr == MAP_FAILED) {
                perror("mmap failed during page fault handling");
                exit(1);
            }

            // Copy the content of the segment into the mapped memory
            size_t offset = page_addr - phdr[i].p_vaddr;
            size_t file_sz = phdr[i].p_filesz;
            size_t load_sz = phdr[i].p_memsz;
            size_t chunk_size = load_sz - offset;

            if (chunk_size > PAGE_SIZE) {
                chunk_size = PAGE_SIZE;
            }

            if (offset + chunk_size <= file_sz) {
                // Read the segment's file data into the mapped page
                pread(fd, mapped_addr, chunk_size, phdr[i].p_offset + offset);
            } else {
                // Zero out the remaining space on the page
                memset(mapped_addr, 0, PAGE_SIZE);
            }

            page_fault_count++;
            return;
        }
    }

    // If no valid segment found, treat as a regular segmentation fault
    printf("Segmentation fault at address %p\n", fault_addr);
    exit(1);
}

// Release memory and other cleanups
void loader_cleanup() {
    if (fd >= 0) close(fd);
    if (ehdr) free(ehdr);
    if (phdr) free(phdr);
    if (segments) free(segments);
}

// Load and run the ELF executable file
void load_and_run_elf(const char** exe) {
    // Register the signal handler for segmentation faults
    signal(SIGSEGV, segfault_handler); // Simpler signal handling

    fd = open(exe[1], O_RDONLY);
    if (fd < 0) {
        perror("Failed to open the ELF file\n");
        loader_cleanup();
        exit(1);
    }

    // 1. Load the ELF header
    ehdr = (Elf32_Ehdr*)malloc(sizeof(Elf32_Ehdr));
    if (read(fd, ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) {
        perror("Failed to read ELF header\n");
        loader_cleanup();
        exit(1);
    }

    off_t e_phoff = ehdr->e_phoff;
    uint16_t e_phnum = ehdr->e_phnum;

    // 2. Allocate memory for program headers and read them
    phdr = (Elf32_Phdr*)malloc(e_phnum * sizeof(Elf32_Phdr));
    lseek(fd, e_phoff, SEEK_SET);
    if (read(fd, phdr, e_phnum * sizeof(Elf32_Phdr)) != (e_phnum * sizeof(Elf32_Phdr))) {
        perror("Failed to read program headers\n");
        loader_cleanup();
        exit(1);
    }

    // 3. Lazy-load segments
    segments = calloc(e_phnum, sizeof(void*)); // To store segment pointers for cleanup

    // 4. Setup entry point
    for (int i = 0; i < e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            // Just map the first page initially, other pages will be mapped upon fault
            void *first_page = mmap((void *)phdr[i].p_vaddr, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC,
                                    MAP_PRIVATE, fd, phdr[i].p_offset);
            if (first_page == MAP_FAILED) {
                perror("Initial mmap failed");
                loader_cleanup();
                exit(1);
            }

            segments[i] = first_page;
        }

        // Check if entry point is within this segment
        if (ehdr->e_entry >= phdr[i].p_vaddr && ehdr->e_entry < phdr[i].p_vaddr + phdr[i].p_memsz) {
            int (*_start)() = (int (*)())(phdr[i].p_vaddr + (ehdr->e_entry - phdr[i].p_vaddr));
            int result = _start();
            printf("User _start return value = %d\n", result);
            break;
        }
    }

    printf("Reached the end of load_and_run_elf\n");
    printf("Total page faults: %zu\n", page_fault_count);
}

int main(int argc, const char** argv) {
    if (argc != 2) {
        printf("Usage: %s <ELF Executable>\n", argv[0]);
        exit(1);
    }
    load_and_run_elf(argv);
    loader_cleanup();
    return 0;
}