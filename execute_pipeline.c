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
#include "execute_pipeline.h"

void execute_pipeline(char*** args, int n, ExecutionContext* ctx, char*** history, int history_cnt) {
    // input for current cmd (start with invalid value as first cmd does not have input)
    int in_fd = -1; 
    pid_t pids[50]; // store all child pids
    int valid_pids = 0; // num of childs
    pid_t pgid = 0; // process group id

    // loop through cmds
    for (int i = 0; i < n; i++) {
        if (args[i] == NULL || args[i][0] == NULL) continue;

        int pipefd[2];

        // to pass stdout to the next process (if exits)
        bool has_next = false;
        if (i < n - 1) {
            for (int j = i + 1; j < n; j++) {
                if (args[j] != NULL && args[j][0] != NULL) {
                    has_next = true;
                    break;
                }
            }
        }

        if (has_next) {
            if (pipe(pipefd) < 0) {
                perror("pipe");
                return;
            }
        }

        pid_t pid = fork();
        if (pid < 0) { perror("fork"); return; }

        if (pid == 0) {
            // child process

            // ignore default behaviour of ctrl+z/c in the shell
            // exec correctly in the child
            signal(SIGTSTP, SIG_DFL);
            signal(SIGINT, SIG_DFL);

            // read from previous cmd (if not the first one)
            if (in_fd != -1) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            
            // write to the next command (if exists)
            if (has_next) {
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[0]);
                close(pipefd[1]);
            }

            // first command input redirection
            if (i == 0 && ctx[i].input_file) {
                int fd = open(ctx[i].input_file, O_RDONLY);
                if (fd < 0) { perror("input file"); exit(1); }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }

            // last command output redirection
            if (i == n - 1 && ctx[i].output_file) {
                int fd = open(ctx[i].output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) { perror("output file"); exit(1); }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            // built-ins (only those that make sense in a child)
            if (builtin_cd_exit(args[i])) exit(0);
            if (builtin_pwd_history(args[i], history, history_cnt)) exit(0);

            execvp(args[i][0], args[i]);
            perror("execvp");
            exit(1);
        } else {
            // parent process
            if (pgid == 0) {

                // create process group
                // first process is the group leader
                pgid = pid;
                setpgid(pid, pgid);

                // receive signal by the entire group (not single process)
                if (!ctx[0].is_background && isatty(STDIN_FILENO)) {
                    if (tcsetpgrp(STDIN_FILENO, pgid) < 0) {
                        perror("tcsetpgrp failed in pipeline");
                    }
                }
            } else {

                // put other processes in the same group
                setpgid(pid, pgid);
            }

            // store pids in our list
            pids[valid_pids++] = pid;

            if (in_fd != -1) close(in_fd);

            // current read, becomes the next input
            if (has_next) {
                close(pipefd[1]);
                in_fd = pipefd[0];
            }
        }
    }

    if (in_fd != -1) close(in_fd);

    if (ctx[0].is_background) {
        printf("[pipeline running in background]\n");
        return;
    }

    // wait for all processes in the pipeline
    for (int i = 0; i < valid_pids; i++) {

        // wait for termination or stop (Ctrl+Z)
        int status;
        pid_t result = waitpid(pids[i], &status, WUNTRACED);

        if (result == -1) {
            perror("waitpid in pipeline");
        } else if (WIFSTOPPED(status)) {

            // WSTOPSIG(status) => signal that stopped the process
            printf("\n[%d]  + Stopped (signal %d)\n",
                   pids[i], WSTOPSIG(status));
        }
    }

    // Reclaim terminal control if interactive
    if (isatty(STDIN_FILENO)) {
        signal(SIGTTOU, SIG_IGN);

        // give terminal back to the shell from the process
        if (tcsetpgrp(STDIN_FILENO, getpgid(0)) < 0) {
            perror("tcsetpgrp failed restoring shell control in pipeline");
        }
    }
}