#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <stdbool.h>
#include <fcntl.h>

#define BUFFER_SIZE 100
#define HISTORY_SIZE 100
#define MAX_CPU 4
#define OUTPUT_SIZE 1000

typedef struct {
    char *command;
    int pid;
    struct timeval start_time;
    struct timeval end_time;
    long duration;
    long wait_time;
    char output[OUTPUT_SIZE]; // Store the output of the command
} command_info;

command_info command_history[HISTORY_SIZE];
int history_count = 0;
int NCPU = 1; // Number of CPU cores
int TSLICE = 1000; // Time slice in milliseconds
int ready_queue[HISTORY_SIZE];
int queue_count = 0;//number if ekements
bool running = true;
bool ctrl_c_flag = false;

// Function to display command information
void display_command_info() {
    printf("\nDisplaying command info...\n");
    for (int i = 0; i < history_count; i++) {
        printf("Command: %s\n", command_history[i].command);
        printf("PID: %d\n", command_history[i].pid);
        printf("Start time: %ld.%06ld\n", (long)command_history[i].start_time.tv_sec, (long)command_history[i].start_time.tv_usec);
        printf("Completion time: %ld.%06ld\n", (long)command_history[i].end_time.tv_sec, (long)command_history[i].end_time.tv_usec);
        printf("Wait time: %ld ms\n", command_history[i].duration);
        //printf("Wait time: %ld ms\n", command_history[i].wait_time);
        printf("Output:%s\n", command_history[i].output);
        printf("------------------------\n");
        free(command_history[i].command); // Free allocated memory
    }
}

// Signal handler for SIGINT
void sigint_handler(int sig_num) {
    ctrl_c_flag = true;
    running = false;
}

// Signal handler for child process termination
void sigchld_handler(int sig_num) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < history_count; i++) {
            if (command_history[i].pid == pid) {
                struct timeval end;
                gettimeofday(&end, NULL);
                command_history[i].end_time = end;
                command_history[i].duration = (end.tv_sec - command_history[i].start_time.tv_sec) * 1000 + (end.tv_usec - command_history[i].start_time.tv_usec) / 1000;

                // Read output from the pipe
                char buffer[OUTPUT_SIZE];
                int n = read(command_history[i].output[0], buffer, OUTPUT_SIZE - 1);
                if (n > 0) {
                    buffer[n] = '\0';
                    strncpy(command_history[i].output, buffer, OUTPUT_SIZE - 1);
                }
                close(command_history[i].output[0]); // Close read end of the pipe

                break;
            }
        }
    }
}

// function to execute a command
void execute_command(char *command) {
    int output_pipe[2];
    if (pipe(output_pipe) == -1) {
        perror("Error creating pipe\n");
        exit(1);
    }

    int id = fork();
    if (id < 0) {
        perror("Error during fork\n");
        exit(1);
    }
    else if (id == 0) {
        close(output_pipe[0]); // Close read end of the pipe in the child
        dup2(output_pipe[1], STDOUT_FILENO); // Redirect stdout to the pipe
        dup2(output_pipe[1], STDERR_FILENO); // Redirect stderr to the pipe
        close(output_pipe[1]); // Close write end of the pipe in the child

        char *args[BUFFER_SIZE];
        char *token = strtok(command, " ");
        int i = 0;
        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;
        pause(); // wait for signal from SimpleScheduler
        if (execvp(args[0], args) == -1) {
            perror("Execution failed\n");
            exit(1);
        }
    }
    else {
        close(output_pipe[1]); // Close write end of the pipe in the parent

        command_history[history_count].pid = id;
        command_history[history_count].command = strdup(command);
        gettimeofday(&command_history[history_count].start_time, NULL);
        command_history[history_count].duration = -1;
        command_history[history_count].wait_time = 0;
        ready_queue[queue_count++] = id;
        command_history[history_count].output[0] = output_pipe[0];
        history_count++;
    }
}
void scheduler(int signum) {
    if (!running) return;

    int executing_count = NCPU; // Number of processes to execute at once

    // sending SIGCONT to the first NCPU processes in the queue
    for (int i = 0; i < executing_count && i < queue_count; i++) {
        kill(ready_queue[i], SIGCONT);
    }

    // Pause the remaining processes
    for (int i = executing_count; i < queue_count; i++) {
        kill(ready_queue[i], SIGSTOP);
    }

    usleep(TSLICE * 1000);  // Time slice wait
    if (running) {
        alarm(TSLICE / 1000);  // Trigger the next scheduling interval
    }
}

// Function to wait for all processes to terminate
void wait_for_all_processes() {
    for (int i = 0; i < history_count; i++) {
        if (command_history[i].duration == -1) {
            waitpid(command_history[i].pid, NULL, 0);
            struct timeval end;
            gettimeofday(&end, NULL);
            command_history[i].end_time = end;
            command_history[i].duration = (end.tv_sec - command_history[i].start_time.tv_sec) * 1000 + (end.tv_usec - command_history[i].start_time.tv_usec) / 1000;

            // Read output from the pipe for any remaining data
            char buffer[OUTPUT_SIZE];
            int n = read(command_history[i].output[0], buffer, OUTPUT_SIZE - 1);
            if (n > 0) {
                buffer[n] = '\0';
                strncat(command_history[i].output, buffer, OUTPUT_SIZE - strlen(command_history[i].output) - 1);
            }
            close(command_history[i].output[0]);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <NCPU> <TSLICE>\n", argv[0]);
        exit(1);
    }
    NCPU = atoi(argv[1]);
    TSLICE = atoi(argv[2]);

    signal(SIGINT, sigint_handler); // Handle Ctrl+C
    signal(SIGALRM, scheduler); // Handle alarms for scheduling
    signal(SIGCHLD, sigchld_handler); // Handle child process termination

    system("clear");
    alarm(TSLICE / 1000); // Start the scheduler

    while (running) {
        printf("SimpleShell:~$ ");
        char user_input[BUFFER_SIZE];
        if (fgets(user_input, BUFFER_SIZE, stdin) == NULL) break;
        int user_input_length = strlen(user_input);
        if (user_input[user_input_length - 1] == '\n') {
            user_input[user_input_length - 1] = '\0';
        }
        if (strcmp(user_input, "history") == 0) {
            for (int i = 0; i < history_count; i++) {
                printf("%d: %s\n", i + 1, command_history[i].command);
            }
            continue;
        }
        else if (strncmp(user_input, "submit ", 7) == 0) {
            execute_command(user_input + 7);
        }
    }

    wait_for_all_processes();

    if (ctrl_c_flag) {
        display_command_info();
    }

    return 0;
}

