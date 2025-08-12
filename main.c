#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

// --- Definitions and Job Control Structures ---
#define MAX_JOBS 20
#define MAX_CMD_LEN 1024
#define TOKEN_BUFSIZE 64
#define TOKEN_DELIM " \t\r\n\a"

typedef struct {
    pid_t pid;
    int id;
    char command[MAX_CMD_LEN];
    char status; // 'r': running, 's': stopped, 'd': done
} job_t;

job_t job_list[MAX_JOBS];

// --- Forward Declarations ---
void shell_loop(void);
void reap_jobs(void);
void sigchld_handler(int sig);
int execute_args(char **args, const char* line);
char* read_line(void);
char** parse_line(char *line);
int my_cd(char **args);
int my_help(char **args);
int my_exit(char **args);
int my_jobs(char **args);

// --- Built-in Commands ---
char *builtin_str[] = {"cd", "help", "exit", "jobs"};
int (*builtin_func[]) (char **) = {&my_cd, &my_help, &my_exit, &my_jobs};

int num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}

// --- Main Entry Point ---
int main(int argc, char **argv) {
    signal(SIGCHLD, sigchld_handler);
    for (int i = 0; i < MAX_JOBS; i++) {
        job_list[i].id = 0;
    }
    shell_loop();
    return EXIT_SUCCESS;
}

// --- Main Shell Loop ---
void shell_loop(void) {
    char *line;
    char **args;
    int status;
    do {
        reap_jobs();
        printf("-> ");
        line = read_line();
        
        char command_copy[MAX_CMD_LEN];
        strncpy(command_copy, line, MAX_CMD_LEN);
        command_copy[strcspn(command_copy, "\n")] = 0;

        args = parse_line(line);
        status = execute_args(args, command_copy);

        free(line);
        free(args);
    } while (status);
}

// --- Job Management and Signal Handling ---
void add_job(pid_t pid, const char* command) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (job_list[i].id == 0) {
            job_list[i].pid = pid;
            job_list[i].id = i + 1;
            strncpy(job_list[i].command, command, MAX_CMD_LEN);
            job_list[i].status = 'r';
            printf("[%d] %d\n", job_list[i].id, pid);
            return;
        }
    }
    printf("myshell: too many jobs\n");
}

void sigchld_handler(int sig) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < MAX_JOBS; i++) {
            if (job_list[i].id != 0 && job_list[i].pid == pid) {
                job_list[i].status = 'd';
                break;
            }
        }
    }
}

void reap_jobs() {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (job_list[i].id != 0 && job_list[i].status == 'd') {
            printf("[%d] Done\t%s\n", job_list[i].id, job_list[i].command);
            job_list[i].id = 0;
        }
    }
}

int my_jobs(char **args) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (job_list[i].id != 0 && job_list[i].status != 'd') {
            char status_str[20];
            if (job_list[i].status == 'r') {
                strcpy(status_str, "Running");
            } else if (job_list[i].status == 's') {
                strcpy(status_str, "Stopped");
            }
            printf("[%d] %s\t%s\n", job_list[i].id, status_str, job_list[i].command);
        }
    }
    return 1;
}

// --- Core Execution Logic ---
void handle_redirection_and_exec(char** args) {
    char *inputFile = NULL;
    char *outputFile = NULL;
    int i = 0;
    while (args[i] != NULL) {
        if (strcmp(args[i], "<") == 0) { args[i] = NULL; inputFile = args[i + 1]; } 
        else if (strcmp(args[i], ">") == 0) { args[i] = NULL; outputFile = args[i + 1]; }
        i++;
    }
    if (inputFile) {
        int in_fd = open(inputFile, O_RDONLY);
        if (in_fd == -1) { perror("myshell"); exit(EXIT_FAILURE); }
        dup2(in_fd, STDIN_FILENO); close(in_fd);
    }
    if (outputFile) {
        int out_fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out_fd == -1) { perror("myshell"); exit(EXIT_FAILURE); }
        dup2(out_fd, STDOUT_FILENO); close(out_fd);
    }
    if (execvp(args[0], args) == -1) {
        perror("myshell");
    }
    exit(EXIT_FAILURE);
}

// THIS IS THE FIXED FUNCTION
int execute_args(char **args, const char* line) {
    if (args[0] == NULL) return 1;

    for (int i = 0; i < num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            // This line is now correct, it calls the function.
            return (*builtin_func[i])(args);
        }
    }

    int is_background = 0;
    int arg_count = 0;
    while(args[arg_count] != NULL) arg_count++;
    if (arg_count > 0 && strcmp(args[arg_count - 1], "&") == 0) {
        is_background = 1;
        args[arg_count - 1] = NULL;
    }

    int pipe_pos = -1;
    for (int i = 0; i < arg_count; i++) {
        if (args[i] != NULL && strcmp(args[i], "|") == 0) { pipe_pos = i; break; }
    }
    
    if (pipe_pos == -1) { // NO PIPE
        pid_t pid = fork();
        if (pid == 0) {
            handle_redirection_and_exec(args);
        } else if (pid < 0) {
            perror("myshell");
        } else {
            if (!is_background) {
                waitpid(pid, NULL, 0);
            } else {
                add_job(pid, line);
            }
        }
    } else { // PIPE
        args[pipe_pos] = NULL;
        char **cmd1_args = args;
        char **cmd2_args = &args[pipe_pos + 1];
        int pipe_fds[2];
        pid_t pid1, pid2;

        if (pipe(pipe_fds) < 0) { perror("myshell"); return 1; }
        
        pid1 = fork();
        if (pid1 == 0) {
            dup2(pipe_fds[1], STDOUT_FILENO);
            close(pipe_fds[0]);
            close(pipe_fds[1]);
            handle_redirection_and_exec(cmd1_args);
        }
        
        pid2 = fork();
        if (pid2 == 0) {
            dup2(pipe_fds[0], STDIN_FILENO);
            close(pipe_fds[0]);
            close(pipe_fds[1]);
            handle_redirection_and_exec(cmd2_args);
        }
        
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        
        if(!is_background) {
            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0);
        } else {
            add_job(pid2, line);
        }
    }
    return 1;
}

// --- Utility Implementations ---
char *read_line(void) {
    char *line = NULL;
    size_t bufsize = 0;
    if (getline(&line, &bufsize, stdin) == -1) {
        if (feof(stdin)) { exit(EXIT_SUCCESS); } 
        else { perror("read_line"); exit(EXIT_FAILURE); }
    }
    return line;
}

char **parse_line(char *line) {
    int bufsize = TOKEN_BUFSIZE;
    int position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;
    if (!tokens) { fprintf(stderr, "myshell: allocation error\n"); exit(EXIT_FAILURE); }
    token = strtok(line, TOKEN_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;
        if (position >= bufsize) {
            bufsize += TOKEN_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) { fprintf(stderr, "myshell: allocation error\n"); exit(EXIT_FAILURE); }
        }
        token = strtok(NULL, TOKEN_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

int my_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "myshell: expected argument to \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("myshell");
        }
    }
    return 1;
}

int my_exit(char **args) {
    return 0;
}

int my_help(char **args) {
    printf("My Custom Shell - by [Your Name]\n");
    printf("Supports pipes, I/O redirection, and basic job control.\n");
    printf("Built-in commands: cd, help, exit, jobs\n");
    return 1;
}
