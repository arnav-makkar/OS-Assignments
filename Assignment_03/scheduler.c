#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>

#define BUFFER_SIZE 100
#define HISTORY_SIZE 100

typedef struct {
    char* command;
    int pid;
    struct timeval start_time;
    long duration;
    bool completed;
} command_info;

command_info command_history[HISTORY_SIZE];
int history_count = 0;

int NCPU;
int TSLICE;
int ready_queue[HISTORY_SIZE];
int rq_front = 0, rq_rear = -1, rq_size = 0;
pid_t scheduler_pid;

void enqueue(int pid) {
    ready_queue[++rq_rear % HISTORY_SIZE] = pid;
    rq_size++;
}

int dequeue() {
    if (rq_size == 0) return -1;
    int pid = ready_queue[rq_front++ % HISTORY_SIZE];
    rq_size--;
    return pid;
}

void sigint_handler(int sig_num) {
    printf("\nReceived Ctrl+C. Displaying command info...\n");

    for (int i = 0; i < history_count; i++) {
        printf("Command: %s\n", command_history[i].command);
        printf("PID: %d\n", command_history[i].pid);
        printf("Start time: %ld.%06ld\n", (long)command_history[i].start_time.tv_sec, (long)command_history[i].start_time.tv_usec);

        if (command_history[i].duration != -1) {
            printf("Duration: %ld ms\n", command_history[i].duration);
        } else {
            printf("Duration: Background process still running\n");
        }
        printf("------------------------\n");
    }

    for (int i = 0; i < history_count; i++) {
        free(command_history[i].command);
    }

    kill(scheduler_pid, SIGTERM);
    exit(0);
}

void execute_command(char* command, bool is_bg_cmd) {
    int id = fork();

    if (id < 0) {
        perror("Fork failed\n");
        exit(1);
    }
    else if (id == 0) {
        // Child process (Job)
        pause();
        
        char *args[BUFFER_SIZE];
        char *token = strtok(command, " ");
        int i = 0;
        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        if (execvp(args[0], args) == -1) {
            perror("Execution failed\n");
            exit(1);
        }
    } else {
        // Parent process (Shell)
        command_history[history_count].pid = id;
        command_history[history_count].command = strdup(command);
        gettimeofday(&command_history[history_count].start_time, NULL);
        command_history[history_count].duration = -1;
        command_history[history_count].completed = false;
        
        enqueue(id);
        history_count++;

        if (!is_bg_cmd) {
            printf("Submitted process (PID: %d) is now in ready queue\n", id);
        }
    }
}

void simple_scheduler() {
    struct itimerval timer;
    timer.it_value.tv_sec = TSLICE / 1000;
    timer.it_value.tv_usec = (TSLICE % 1000) * 1000;
    timer.it_interval = timer.it_value;

    while (1) {
        for (int i = 0; i < NCPU && rq_size > 0; i++) {
            int pid = dequeue();
            if (pid > 0) {
                kill(pid, SIGCONT);
            }
        }

        usleep(TSLICE * 1000);

        for (int i = 0; i < NCPU && rq_size > 0; i++) {
            int pid = dequeue();
            if (pid > 0) {
                kill(pid, SIGSTOP);
                enqueue(pid);
            }
        }
    }
}

void start_scheduler() {
    scheduler_pid = fork();

    if (scheduler_pid < 0) {
        perror("Scheduler fork failed");
        exit(1);
    }
    else if (scheduler_pid == 0) {
        simple_scheduler();
    }
}

bool check_if_bg(char *command) {
    int len = strlen(command);
    if (len > 0 && command[len - 1] == '&') {
        command[len - 1] = '\0';
        return true;
    }
    return false;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <NCPU> <TSLICE(ms)>\n", argv[0]);
        exit(1);
    }

    NCPU = atoi(argv[1]);
    TSLICE = atoi(argv[2]);

    if (NCPU <= 0 || TSLICE <= 0) {
        fprintf(stderr, "Error: NCPU and TSLICE must be positive integers\n");
        exit(1);
    }

    signal(SIGINT, sigint_handler);

    start_scheduler();

    while (1) {
        printf("SimpleShell$ ");

        char user_input[BUFFER_SIZE];
        if (fgets(user_input, BUFFER_SIZE, stdin) == NULL) {
            break;
        }

        user_input[strcspn(user_input, "\n")] = 0;

        if (strncmp(user_input, "submit", 6) == 0) {
            char *command = user_input + 7;
            bool is_bg_cmd = check_if_bg(command);
            execute_command(command, is_bg_cmd);
        }
        else if (strcmp(user_input, "history") == 0) {
            for (int i = 0; i < history_count; i++) {
                printf("%d: %s\n", i + 1, command_history[i].command);
            }
        }
    }

    for (int i = 0; i < history_count; i++) {
        free(command_history[i].command);
    }

    return 0;
}

