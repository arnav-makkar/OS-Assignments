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
    #include <sys/types.h>
    #include <unistd.h>

    // #define _XOPEN_SOURCE 700
    //#define _POSIX_C_SOURCE 199309L


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
    // Signal handler for segmentation fault (page fault)
    void sigsegv_handler(int sig, siginfo_t *si, void *unused) {
        void *fault_addr = si->si_addr;
        // uintptr_t *seg_start = NULL;
        void *seg_end = NULL;
        size_t segment_offset = 0;
        int segment_index = -1;
        // printf("here %d\n", fault_addr);
        // Find the segment that contains the fault address
        for (int i = 0; i < ehdr->e_phnum; i++) {
            
            if (phdr[i].p_type == PT_LOAD) {
                if ((uintptr_t)fault_addr >= phdr[i].p_vaddr && (uintptr_t)fault_addr < phdr[i].p_vaddr + phdr[i].p_memsz)
                {
                    //printf("%d -> %d",phdr[i].p_vaddr, phdr[i].p_memsz);
                    uintptr_t seg_start = ((uintptr_t)fault_addr/PAGE_SIZE)*PAGE_SIZE;

                    void *mapped_page = mmap((void*)seg_start, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC,
                                        MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
                    if (mapped_page == MAP_FAILED) {
                        perror("mmap");
                        exit(1);
                    }
                    //calculation for total internal fragmentation
                    // if(phdr[i].p_vaddr+phdr[i].p_filesz-seg_start<PAGE_SIZE){
                    //         total_fragmentation+=PAGE_SIZE-(phdr[i].p_vaddr+phdr[i].p_filesz-seg_start);
                    // }
                    if (seg_start + PAGE_SIZE >= phdr[i].p_vaddr + phdr[i].p_memsz) {
                        size_t used_bytes_in_last_page = (phdr[i].p_vaddr + phdr[i].p_memsz) % PAGE_SIZE;
                        size_t internal_fragmentation = PAGE_SIZE - used_bytes_in_last_page;
                        total_fragmentation += internal_fragmentation;
                    }
                    size_t offset_in_seg = seg_start - phdr[i].p_vaddr;
                    printf("start offset = %d\n", phdr[i].p_offset+offset_in_seg);
                    lseek(fd, phdr[i].p_offset +offset_in_seg, SEEK_SET);
                    read(fd, mapped_page, PAGE_SIZE);
                    page_alloc_count++;
                    page_fault_count++;
                    return;
                
                }
            }

        //     break;
        }
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
        int (*_start)() = (int (*)())((ehdr->e_entry ));
        int result = _start();
        printf("User _start return value = %d\n", result);
        // 6. Report statistics after execution
        printf("\nProgram execution completed.\n");
        printf("Total page faults: %zu\n", page_fault_count);
        printf("Total page allocations: %zu\n", page_alloc_count);
        printf("Total internal fragmentation: %zu bytes\n", total_fragmentation);

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
