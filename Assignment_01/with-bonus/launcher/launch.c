#include "loader.h"
#include <loader.c>
void load_and_run_elf();
void loader_cleanup();
int main(int argc, char** argv) 
{
  if(argc != 2) {
    printf("Usage: %s <ELF Executable> \n",argv[0]);
    exit(1);
  }
  const char* ag = argv[1];
  load_and_run_elf(ag);
  loader_cleanup();
  return 0;
}
