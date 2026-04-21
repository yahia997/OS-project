#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ExecutionContext.h"
#include <stdbool.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "builtin.h"
#include <signal.h>
#include "execute_single_command.h"

void execute_single_command(char** args, ExecutionContext* ctx, char*** history, int history_cnt) {
    // create new process
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return;
    }

    if (pid == 0) {
        // child process
        signal(SIGTSTP, SIG_DFL);   // allow ctrl+Z to stop child
        signal(SIGINT, SIG_DFL);    // allow ctrl+C to interrupt child

        // input redirection
        if (ctx->input_file) {
            // using open system call to open the file for read only
            int fd = open(ctx->input_file, O_RDONLY);

            if (fd < 0) {
                perror("input file");
                exit(1);
            }

            // put the standard input into the file
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        // output redirection
        if (ctx->output_file) {
            // O_WRONLY => write only
            // O_CREAT => create if the file is not found
            // O_TRUNC => clear content of the file if it exists
            // 0644 = rw-r--r-- (file priviledges)
            int fd = open(ctx->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("output file");
                exit(1);
            }

            // put standard output in the file
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        // handle built-in commands that must run in child (pwd, history)
        if (builtin_pwd_history(args, history, history_cnt)) exit(0);

        // if no builtin cmd executed exec using execvp
        execvp(args[0], args);
        perror(args[0]); // show error message
        exit(EXIT_FAILURE); // exit the current process
    } else {
        // parent process

        // create new process group
        // place child in its own process group
        // child becomes leader of its group
        // without process group we can not control signals ctrl+c/z
        // shell => in a group, child => diff group to not terminate both by ctrl+c
        setpgid(pid, pid);  

        // give terminal control to child if foreground and interactive
        if (!ctx->is_background && isatty(STDIN_FILENO)) {

            //gives terminal control to a process group
            if (tcsetpgrp(STDIN_FILENO, pid) < 0) {
                perror("tcsetpgrp failed in single command");
            }
        }

        if (ctx->is_background) { 
            // print pid if background exec
            // do not wait
            printf("[PID %d]\n", pid);
        } else {
            int status;

            // wait for termination or stop (Ctrl+Z)
            pid_t result = waitpid(pid, &status, WUNTRACED);

            if (result == -1) {
                perror("waitpid");
            } else if (WIFSTOPPED(status)) { // if stopped using ctrl+z

                // WSTOPSIG(status) => signal that stopped the process
                printf("\n[%d]  + Stopped (signal %d)  %s\n",
                       pid, WSTOPSIG(status), args[0]);
            }

            // reclaim terminal control if interactive
            if (isatty(STDIN_FILENO)) {

                // SIGTTOU => stops bg process from writing to terminal
                signal(SIGTTOU, SIG_IGN);

                if (tcsetpgrp(STDIN_FILENO, getpgid(0)) < 0) {
                    perror("tcsetpgrp failed restoring shell control");
                }
            }
        }
    }
}