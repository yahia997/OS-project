#include "Command.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// override run and help functions
static void run(Command* self, char** args, bool background) {
  (void)self;
  (void)args;
  (void)background;
  char cwd[1024];

  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    printf("%s\n", cwd);
  } else {
    perror("getcwd() error");
  }
}

static void help(Command* self) {
  (void)self;
  printf("pwd => print working directory\n");
  printf("To use type: pwd\n");
}

Command* pwd_command() {
  Command* cmd = (Command*)malloc(sizeof(Command));

  cmd->name = (char*)malloc(strlen("pwd") + 1);
  strcpy(cmd->name, "pwd");
  cmd->max_args = 0;
  cmd->run = run;      
  cmd->help = help;    

  return cmd;
}
