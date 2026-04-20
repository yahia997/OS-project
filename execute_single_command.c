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
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return;
    }
    if (pid == 0) {
        // Child process
        signal(SIGTSTP, SIG_DFL);   // Allow Ctrl+Z to stop child
        signal(SIGINT, SIG_DFL);    // Allow Ctrl+C to interrupt child

        // Input redirection
        if (ctx->input_file) {
            int fd = open(ctx->input_file, O_RDONLY);
            if (fd < 0) {
                perror("input file");
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        // Output redirection
        if (ctx->output_file) {
            int fd = open(ctx->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("output file");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        // Handle built-in commands that must run in child (pwd, history)
        if (builtin_pwd_history(args, history, history_cnt)) exit(0);

        // External command
        execvp(args[0], args);
        perror(args[0]);
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        setpgid(pid, pid);  // Place child in its own process group

        // Give terminal control to child if foreground and interactive
        if (!ctx->is_background && isatty(STDIN_FILENO)) {
            if (tcsetpgrp(STDIN_FILENO, pid) < 0) {
                perror("tcsetpgrp failed in single command");
            }
        }

        if (ctx->is_background) {
            printf("[PID %d]\n", pid);
        } else {
            int status;
            // Wait for termination OR stop (Ctrl+Z)
            pid_t result = waitpid(pid, &status, WUNTRACED);
            if (result == -1) {
                perror("waitpid");
            } else if (WIFSTOPPED(status)) {
                printf("\n[%d]  + Stopped (signal %d)  %s\n",
                       pid, WSTOPSIG(status), args[0]);
            }

            // Reclaim terminal control if interactive
            if (isatty(STDIN_FILENO)) {
                signal(SIGTTOU, SIG_IGN);
                if (tcsetpgrp(STDIN_FILENO, getpgid(0)) < 0) {
                    perror("tcsetpgrp failed restoring shell control");
                }
            }
        }
    }
}