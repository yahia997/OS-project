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
#include "add_to_history.h"

void add_to_history(char*** history, int* history_cnt, char* command) {
    if (*history_cnt >= 1000) return;

    history[*history_cnt] = (char**)malloc(2 * sizeof(char*));
    if (history[*history_cnt] == NULL) {
        perror("malloc failed for history entry");
        return;
    }

    history[*history_cnt][0] = strdup(command);
    if (history[*history_cnt][0] == NULL) {
        perror("strdup failed");
        free(history[*history_cnt]);
        return;
    }

    history[*history_cnt][1] = NULL;
    (*history_cnt)++;
}