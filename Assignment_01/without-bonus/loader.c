#include "loader.h"

Elf32_Ehdr *ehdr = NULL;
Elf32_Phdr *phdr = NULL;
int fd = -1;
void *virtual_mem = NULL;

// Release memory and other cleanups
void loader_cleanup() {
	if(fd >= 0) close(fd);
	if(ehdr) free(ehdr);
	if(phdr) free(phdr);
	if (virtual_mem) munmap(virtual_mem, phdr->p_memsz);
}

// Load and run the ELF executable file
void load_and_run_elf(char** exe){
  	fd = open(exe[0], O_RDONLY);

  	// 1. Load entire binary content into the memory from the ELF file.
	if(fd < 0){
		printf("Failed to open the ELF file");
		loader_cleanup();
		exit(1);
	}

	ehdr = (Elf32_Ehdr*)malloc(sizeof(Elf32_Ehdr));
	if(read(fd, ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)){
		printf("Failed to read ELF header");
		loader_cleanup();
		exit(1);
	}

	off_t e_phoff = ehdr->e_phoff;
	size_t e_phentsize = ehdr->e_phentsize;
	uint16_t e_phnum = ehdr->e_phnum;

	// 2. Iterate through the PHDR table and find the section of PT_LOAD type that contains the address of the entrypoint method in fib.c

	phdr = (Elf32_Phdr*)malloc(e_phnum * sizeof(Elf32_Phdr));
	lseek(fd, e_phoff, SEEK_SET);
	if(read(fd, phdr, e_phnum*sizeof(Elf32_Phdr)) != (e_phnum*sizeof(Elf32_Phdr))){
		printf("Failed to read program headers");
		loader_cleanup();
		exit(1);
	}

	//printf("cp1\n");

	// 3. Allocate memory of the size "p_memsz" using mmap function and then copy the segment content
	// 4. Navigate to the entrypoint address into the segment loaded in the memory in above step

	lseek(fd, e_phoff, SEEK_SET);
	for (int i = 0; i < e_phnum; i++) {
		
        Elf32_Phdr phdr;
        if (read(fd, &phdr, sizeof(phdr)) != sizeof(phdr)) {
            printf("Error");
            exit(1);
        }

		if (phdr.p_type == PT_LOAD){

			//printf("ptload found\n");

			virtual_mem = mmap(NULL, phdr.p_memsz, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);

			if (virtual_mem == MAP_FAILED) {
				printf("mmap failed");
				loader_cleanup();
				exit(1);
			}

			lseek(fd, phdr.p_offset, SEEK_SET);
			if (read(fd, virtual_mem, phdr.p_filesz) != phdr.p_filesz) {
				perror("Failed to load segment");
				loader_cleanup();
				exit(1);
			}

			size_t offset = ehdr->e_entry - phdr.p_vaddr;
			int (*_start)() = (int (*)())((char *)virtual_mem + offset);
			int result = _start();
			printf("User _start return value = %d\n",result);

			break;
        }
	}

	// 5. Typecast the address to that of function pointer matching "_start" method in fib.c.
	// 6. Call the "_start" method and print the value returned from the "_start"


	//printf("cp3\n");
}

int main(int argc, char** argv){
	if(argc != 2) {
		printf("Usage: %s <ELF Executable> \n",argv[0]);
		exit(1);
	}

	load_and_run_elf(argv);

	loader_cleanup();
	return 0;
}