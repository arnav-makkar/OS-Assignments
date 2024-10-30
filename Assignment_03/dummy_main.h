#ifndef DUMMY_MAIN_H
#define DUMMY_MAIN_H
#include <signal.h>
#include <unistd.h>
// Define dummy_main function signature
int dummy_main(int argc, char **argv);

// Redefine main to dummy_main
#define main dummy_main

// New main definition to support SimpleScheduler
int main(int argc, char **argv) {
    // Custom code to support scheduling:
    signal(SIGCONT, SIG_DFL);  // Ensure SIGCONT resumes execution
    pause();  // Wait until SIGCONT to start

    // Call the actual program’s main function, now renamed to dummy_main
    int ret = dummy_main(argc, argv);
    return ret;
}

#endif // DUMMY_MAIN_H

