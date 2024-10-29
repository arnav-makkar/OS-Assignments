#ifndef DUMMY_MAIN_H
#define DUMMY_MAIN_H

int dummy_main(int argc, char **argv);

int main(int argc, char **argv) {
    /* You can add any code here you want to support your SimpleScheduler implementation */
    pause();  // Wait until SimpleScheduler sends SIGCONT to continue

    int ret = dummy_main(argc, argv);
    return ret;
}

#define main dummy_main

#endif // DUMMY_MAIN_H
