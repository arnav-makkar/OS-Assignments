#include <signal.h>
#include <unistd.h>
// Define dummy_main function signature
int dummy_main(int argc, char **argv);

int main(int argc, char **argv) {
    // Custom code to support scheduling:
    signal(SIGCONT, SIG_DFL);  // Ensure SIGCONT resumes execution
    pause();  // Wait until SIGCONT to start

    // Call the actual program’s main function, now renamed to dummy_main
    int ret = dummy_main(argc, argv);
    return ret;
}
#define main dummy_main

