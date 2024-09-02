#include "loader.h"

Elf32_Ehdr *ehdr = NULL;
Elf32_Phdr *phdr = NULL;
int fd = -1;
void *virtual_mem = NULL;

// Release memory and other cleanups
void loader_cleanup() {
    if (fd >= 0) close(fd);
    if (ehdr) free(ehdr);
    if (phdr) free(phdr);
    if (virtual_mem && phdr) munmap(virtual_mem, phdr->p_memsz);
}

// Load and run the ELF executable file
void load_and_run_elf(const char** exe) {
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
    size_t e_phentsize = ehdr->e_phentsize;
    uint16_t e_phnum = ehdr->e_phnum;

    // 2. Allocate memory for program headers and read them
    phdr = (Elf32_Phdr*)malloc(e_phnum * sizeof(Elf32_Phdr));
    lseek(fd, e_phoff, SEEK_SET);
    if (read(fd, phdr, e_phnum * sizeof(Elf32_Phdr)) != (e_phnum * sizeof(Elf32_Phdr))) {
        perror("Failed to read program headers\n");
        loader_cleanup();
        exit(1);
    }

    // 3. Iterate through the program headers and load segments
    for (int i = 0; i < e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            virtual_mem = mmap(NULL, phdr[i].p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC,
                               MAP_PRIVATE, fd, phdr[i].p_offset);
            if (virtual_mem == MAP_FAILED) {
                perror("mmap failed");
                loader_cleanup();
                exit(1);
            }

            // Zero the memory area from p_filesz to p_memsz
            memset(virtual_mem + phdr[i].p_filesz, 0, phdr[i].p_memsz - phdr[i].p_filesz);
            // 4. Check if the entry point is within this segment
            if (ehdr->e_entry >= phdr[i].p_vaddr && ehdr->e_entry < phdr[i].p_vaddr + phdr[i].p_memsz) {
                int (*_start)() = (int (*)())(virtual_mem + (ehdr->e_entry - phdr[i].p_vaddr));
                int result = _start();
                printf("User _start return value = %d\n", result);
                break;
            }
        }
    }

    printf("Reached the end of load_and_run_elf\n");
}

int loader(int argc, const char** argv) {
    if (argc != 2) {
        printf("Usage: %s <ELF Executable>\n", argv[0]);
        exit(1);
    }
    //printf("file is %s", argv[1]);
    //const char* ag = argv[1];
    load_and_run_elf(argv);

    loader_cleanup();
    return 0;
}
