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
#include "execute_single_command.h"
#include "add_to_history.h"
#include "parse_command.h"

int main() {
    setbuf(stdout, NULL);

    // History storage
    char*** history = (char***)malloc(1000 * sizeof(char**));
    int history_cnt = 0;

    // Place shell in its own process group
    pid_t shell_pgid = getpid();
    if (setpgid(shell_pgid, shell_pgid) < 0) {
        perror("Couldn't put shell in its own process group");
        exit(1);
    }

    // Take control of terminal if interactive
    if (isatty(STDIN_FILENO)) {
        tcsetpgrp(STDIN_FILENO, shell_pgid);
    }

    // Shell ignores these signals
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);

    // Main REPL
    while (1) {
        if (isatty(STDIN_FILENO)) {
            printf("$ ");
        }

        char command[1024];
        if (fgets(command, sizeof(command), stdin) == NULL) {
            printf("\n");
            break;
        }

        command[strcspn(command, "\n")] = '\0';
        if (strlen(command) == 0) continue;

        char command_copy[1024];
        strcpy(command_copy, command);

        add_to_history(history, &history_cnt, command);

        // Split by pipe
        char* input_commands[50];
        int cmd_count = 0;
        char* cmd = strtok(command_copy, "|");
        while (cmd != NULL && cmd_count < 50) {
            input_commands[cmd_count++] = cmd;
            cmd = strtok(NULL, "|");
        }

        char* args_list[50][100];
        char** args_ptrs[50];
        ExecutionContext ctx_list[50];

        for (int i = 0; i < cmd_count; i++) {
            args_ptrs[i] = args_list[i];
            parse_command(input_commands[i], args_list[i], &ctx_list[i]);
        }

        // Validate pipeline edges
        if (cmd_count > 1) {
            if (args_ptrs[0][0] == NULL) {
                fprintf(stderr, "Error: missing command before pipe\n");
                continue;
            }
            if (args_ptrs[cmd_count - 1][0] == NULL) {
                fprintf(stderr, "Error: missing command after pipe\n");
                continue;
            }
        }

        // Built-in exit/cd (only if single command, no pipe)
        if (cmd_count == 1 && builtin_cd_exit(args_list[0])) continue;

        if (cmd_count == 1) {
            execute_single_command(args_ptrs[0], &ctx_list[0], history, history_cnt);
            continue;
        }

        // Pipeline: background flag applies to whole pipeline
        int pipeline_bg = 0;
        for (int i = 0; i < cmd_count; i++) {
            if (ctx_list[i].is_background) pipeline_bg = 1;
        }
        ctx_list[0].is_background = pipeline_bg;

        execute_pipeline(args_ptrs, cmd_count, ctx_list, history, history_cnt);
    }

    // Free history
    for (int i = 0; i < history_cnt; i++) {
        free(history[i][0]);
        free(history[i]);
    }
    free(history);

    return 0;
}