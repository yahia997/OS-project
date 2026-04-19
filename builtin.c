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

// separate between cd and history as 
int builtin_cd_exit(char** args) {

  if (args == NULL || args[0] == NULL) {
    return 0;
  }

  if (strcmp(args[0], "exit") == 0) {
    exit(0);

  }else if (strcmp(args[0], "cd") == 0) {

    if (args[1] == NULL) {
      fprintf(stderr, "cd: at least one argument\n");
    } else {
      if (chdir(args[1]) != 0) {
        perror("cd");
      }
    }

    return 1;
  }
  return 0;
}


int builtin_pwd_history(char** args, char*** history, int history_cnt) {
// int builtin(char** args) {

  if (args == NULL || args[0] == NULL) {
    return 0;
  }

  if (strcmp(args[0], "pwd") == 0) {
    char cwd[1024];

    if (getcwd(cwd, sizeof(cwd)) != NULL) {
      printf("%s\n", cwd);
      fflush(stdout);
    } else {
      perror("getcwd() error");
    }

    return 1;
  }else if(strcmp(args[0], "history") == 0) {
    for (int i = 0; i < history_cnt; i++) {
      if (history[i] != NULL && history[i][0] != NULL) {
        printf("%d  %s\n", i + 1, history[i][0]);
        fflush(stdout);
      }
    }

    return 1;
  }

  return 0;
}