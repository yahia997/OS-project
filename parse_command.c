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
#include "parse_command.h"

void parse_command(char* cmd, char** args, ExecutionContext* ctx) {
    int i = 0;
    char* token = strtok(cmd, " ");
    while (token != NULL && i < 99) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    ctx->input_file = NULL;
    ctx->output_file = NULL;
    ctx->is_background = 0;

    int write_idx = 0;
    for (int j = 0; j < i; j++) {
        if (args[j] == NULL) continue;

        int len = strlen(args[j]);
        if (len > 0 && args[j][len - 1] == '&') {
            ctx->is_background = 1;
            if (len == 1) continue;
            else args[j][len - 1] = '\0';
        }

        if (strcmp(args[j], "<") == 0) {
            ctx->input_file = args[j + 1];
            j++;
            continue;
        } else if (strcmp(args[j], ">") == 0) {
            ctx->output_file = args[j + 1];
            j++;
            continue;
        } else if (strcmp(args[j], "&") == 0) {
            ctx->is_background = 1;
            continue;
        }

        args[write_idx++] = args[j];
    }
    args[write_idx] = NULL;
}