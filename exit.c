#include "Command.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// override run and help functions
static void run(Command* self, char** args) {
  exit(0);
}

static void help(Command* self) {
  printf("Exit the shell\n");
  printf("To use type: exit\n");
}

Command* exit_command() {
  Command* cmd = (Command*)malloc(sizeof(Command));

  cmd->name = strdup("exit");
  cmd->max_args = 0;
  cmd->run = run;      
  cmd->help = help;    

  return cmd;
}