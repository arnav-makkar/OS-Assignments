#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>

#define BUFFER_SIZE 100
#define HISTORY_SIZE 100

typedef struct {
    char* command;
    int pid;
    struct timeval start_time;
    long duration;
} command_info;
// defined strut for storing command details

command_info command_history[HISTORY_SIZE];
int history_count = 0;

void sigint_handler(int sig_num) {
    printf("\nReceived Ctrl+C. Displaying command info...\n");

    for(int i = 0; i < history_count; i++){
        printf("Command: %s\n", command_history[i].command);
        printf("PID: %d\n", command_history[i].pid);
        printf("Start time: %ld.%06ld\n", (long)command_history[i].start_time.tv_sec, (long)command_history[i].start_time.tv_usec);
        
        if(command_history[i].duration != -1){
            printf("Duration: %ld ms\n", command_history[i].duration);
        }
        else{
            printf("Duration: Background process still running\n");
        }
        printf("------------------------\n");
    }

    for(int i = 0; i<history_count; i++){
        free(command_history[i].command);
    }

    exit(0);
}

int execute_command(char* command, bool bg_cmd){
    int id = fork();
    
    if(id < 0){
        perror("Error occured during forking\n");
        exit(1);
    }
    
    else if(id == 0){
        // child process

        struct timeval start;
        gettimeofday(&start, NULL);

        char *args[BUFFER_SIZE];
        char *token = strtok(command, " ");

        int i = 0;
        while(token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        if(execvp(args[0], args) == -1){
            perror("Execution failed\n");
            exit(1);
        }

        exit(1);
    }
    
    else{
        // parent process

        struct timeval end;
        int status;

        if(bg_cmd == false){

            gettimeofday(&command_history[history_count].start_time, NULL);
            command_history[history_count].pid = id;
            command_history[history_count].command = strdup(command);
            
            waitpid(id, &status, 0);
            gettimeofday(&end, NULL);
            
            command_history[history_count].duration = (end.tv_sec - command_history[history_count].start_time.tv_sec)*1000 + (end.tv_usec - command_history[history_count].start_time.tv_usec)/1000;

            history_count++;
        }
        
        else{
            command_history[history_count].pid = id;
            command_history[history_count].command = strdup(command);
            command_history[history_count].duration = -1;
            history_count++;

            printf("[Background process started] PID: %d\n", id);
            // background pid is printed and loop is continued
        }
    }

    return 1;
}

int execute_piped_commands(char* commands[], int cmd_num){
    int fd[2];
    // fd[0]: read
    // fd[1]: write

    int input_fd = 0; // 0 for stdin

    for (int i = 0; i <cmd_num; i++) {
        if (pipe(fd) == -1) {
            perror("Pipe failed");
            exit(1);
        }

        int id = fork();
        if(id < 0){
            perror("Fork failed");
            exit(1);
        }

        else if (id == 0){
            // child process

            struct timeval start;
            gettimeofday(&start, NULL);

            dup2(input_fd, STDIN_FILENO); // read from the previous pipe/stdin(for first cmd)
            
            if (i < cmd_num-1){
                dup2(fd[1], STDOUT_FILENO); // stdout -> write end
            }

            close(fd[0]);
            close(fd[1]);

            char *args[BUFFER_SIZE];
            char *token = strtok(commands[i], " ");

            int j = 0;
            while(token != NULL){
                args[j++] = token;
                token = strtok(NULL, " ");
            }
            args[j] = NULL;

            if(execvp(args[0], args) == -1){
                perror("Execution failed");
                exit(1);
            }
        }

        else{
            // parent process

            command_history[history_count].pid = id;
            command_history[history_count].command = strdup(commands[i]);
            gettimeofday(&command_history[history_count].start_time, NULL);
            
            waitpid(id, NULL, 0);

            struct timeval end;
            gettimeofday(&end, NULL);
            
            command_history[history_count].duration = (end.tv_sec - command_history[history_count].start_time.tv_sec)*1000 + (end.tv_usec - command_history[history_count].start_time.tv_usec)/1000;
            history_count++;

            close(fd[1]);
            input_fd = fd[0];  // this pipe's read end becomes the input for the next cmd
        }
    }

    return 1;
}

bool check_if_bg(char *command){
    int len = strlen(command);

    if(len>0 && command[len-1] == '&') {
        command[len - 1] = '\0';
        return true;
    }

    return false;
}

int main(int argc, char* argv[]){

    signal(SIGINT, sigint_handler);

    system("clear");

    while(true){

        printf("simple-shell:~$ ");

        char user_input[BUFFER_SIZE];
        if(fgets(user_input, BUFFER_SIZE, stdin) == NULL){
            break;
        }

        int user_input_length = strlen(user_input);
        if(user_input[user_input_length-1] == '\n'){
            user_input[user_input_length-1] = '\0';
        }

        if(user_input_length > 1){
            command_history[history_count].command = strdup(user_input);
            command_history[history_count].duration = -1;
        }

        if(strcmp(user_input, "history") == 0) {
            for (int i = 0; i<history_count; i++) {
                printf("%d: %s\n", i+1, command_history[i].command);
            }

            continue;
        }

        char *cmd_list[BUFFER_SIZE];
        int cmd_num = 0;

        char *token = strtok(user_input, "|");
        while (token != NULL) {
            cmd_list[cmd_num++] = token;
            token = strtok(NULL, "|");
        }

        if(cmd_num > 1){
            execute_piped_commands(cmd_list, cmd_num);
        }

        else{
            bool bg_cmd = check_if_bg(user_input);
            execute_command(user_input, bg_cmd);
        }
    }

    for(int i = 0; i<history_count; i++){
        free(command_history[i].command);
    }

    return 0;
}