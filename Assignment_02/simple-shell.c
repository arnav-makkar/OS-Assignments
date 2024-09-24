#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <signal.h>

#define BUFFER_SIZE 100
#define HISTORY_SIZE 100

char* history[HISTORY_SIZE];
int history_count = 0;

int execute_command(char* command, bool bg_cmd){
    int id = fork();
    
    if(id < 0){
        perror("Error occured during forking\n");
        exit(1);
    }
    
    else if(id == 0){
        // child process

        char *args[BUFFER_SIZE];
        char *token = strtok(command, " ");

        int i = 0;
        while(token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL; // Null-terminate the arguments array

        if(execvp(args[0], args) == -1){
            perror("Execution failed\n");
            exit(1);
        }

        exit(1);
    }
    
    else{
        // parent process

        if(bg_cmd == false){
            int status;
            waitpid(id, &status, 0);

            if(WIFEXITED(status)){
                int statusCode = WEXITSTATUS(status);

                if(statusCode != 0){
                    perror("Failure occured during execution\n");
                    exit(1);
                }
            }
        }
        
        else{
            printf("[Background process started] PID: %d\n", id);
            // background pid is printed and loop is continued
        }
    }

    return 1;
}

int execute_piped_commands(char* cmd1, char* cmd2){
    int fd[2];
    // fd[0]: read
    // fd[1]: write

    if (pipe(fd) == -1) {
        perror("Pipe failed");
        exit(1);
    }

    int id1 = fork();
    if (id1 < 0){
        perror("Fork failed");
        exit(1);
    }
    else if (id1 == 0) {
        // cmd1
        dup2(fd[1], STDOUT_FILENO); // stdout -> write end
        close(fd[0]);
        close(fd[1]);

        char *args1[BUFFER_SIZE];
        char *token = strtok(cmd1, " ");
        int i = 0;
        while (token != NULL) {
            args1[i++] = token;
            token = strtok(NULL, " ");
        }
        args1[i] = NULL;

        if (execvp(args1[0], args1) == -1) {
            perror("Failure occured during execution of cmd1");
            exit(1);
        }
    }

    int id2 = fork();
    if (id2 < 0) {
        perror("Fork failed");
        exit(1);
    }
    else if (id2 == 0) {
        // cmd2
        dup2(fd[0], STDIN_FILENO); // stdin -> read end
        close(fd[1]);
        close(fd[0]);
        
        
        char *args2[BUFFER_SIZE];
        char *token = strtok(cmd2, " ");
        int i = 0;
        while (token != NULL) {
            args2[i++] = token;
            token = strtok(NULL, " ");
        }
        args2[i] = NULL;

        if (execvp(args2[0], args2) == -1) {
            perror("Failure occured during execution of cmd2");
            exit(1);
        }
    }

    close(fd[0]);
    close(fd[1]);
    waitpid(id1, NULL, 0);
    waitpid(id2, NULL, 0);

    return 1;
}

bool check_if_bg(char *command) {
    int len = strlen(command);

    if(len>0 && command[len - 1] == '&') {
        command[len - 1] = '\0';
        return true;
    }

    return false;
}

int main(int argc, char* argv[]){

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
            history[history_count++] = strdup(user_input);
        }

        if(strcmp(user_input, "history") == 0) {
            for (int i = 0; i<history_count; i++) {
                printf("%d: %s\n", i+1, history[i]);
            }

            continue;
        }

        char *pipe_ind = strchr(user_input, '|');

        if(pipe_ind != NULL){
            *pipe_ind = '\0';

            char *cmd1 = user_input;
            char *cmd2 = pipe_ind + 1;

            execute_piped_commands(cmd1, cmd2);
        }

        else{
            bool bg_cmd = check_if_bg(user_input);

            execute_command(user_input, bg_cmd);
        }
    }

    for (int i = 0; i<history_count; i++) {
        free(history[i]);
    }

    return 0;
}

/*
Sample input:

ls
ls /home
echo you should be aware of the plagiarism policy
wc -l fib.c
wc -c fib.c
grep printf helloworld.c
ls -R
ls -l
./fib 40
./helloworld
sort fib.c
uniq file.txt
cat fib.c | wc -l
cat helloworld.c | grep print | wc -l

*/