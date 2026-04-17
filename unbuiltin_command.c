#include "Command.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// override run and help functions
static void run(Command* self, char** args) {
  pid_t pid = fork();

  if (pid < 0) {
    perror("Fork failed");
  } else if (pid == 0) { // child

    execvp(args[0], args);

    // only runs if execvp fails
    perror(args[0]);
    exit(EXIT_FAILURE);
  }else { // parent
    int status;
    waitpid(pid, &status, 0);
  }
}

static void help(Command* self) {
  printf("To run any unbuilt command using fork and execvp\n");
}

Command* unbuiltin_command(char* name) {
  Command* cmd = (Command*)malloc(sizeof(Command));

  cmd->name = name;
  cmd->run = run;      
  cmd->help = help;    

  return cmd;
}