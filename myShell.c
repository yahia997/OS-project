#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Command.h"
#include <stdbool.h>
#include <sys/wait.h>
#include <unistd.h>

// define built in commands
Command* exit_command();
Command* pwd_command();
Command* unbuiltin_command();

static void reap_background_children(void) {
  int status;
  pid_t pid;

  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    (void)pid;
    (void)status;
  }
}

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  // to show output immediately
  setbuf(stdout, NULL);

  // Command list
  Command* commands[] = {
    exit_command(),
    pwd_command(),
    NULL  // Sentinel
  };

  // REPL (Read-Eval-Print Loop)
  while(1) {
    reap_background_children();
    
    // display prompt
    printf("$ ");
    
    // take user commands
    char command[1024];
    if (fgets(command, sizeof(command), stdin) == NULL) {
      printf("\n");
      break;
    }
    
    // so remove trailing new line
    command[strcspn(command, "\n")] = '\0';

    if (command[0] == '\0') {
      continue;
    }

    // list to handle pipes in future
    char* input_commands[50];
    int i = 0;

    char* cmd = strtok(command, "|");
    while (cmd != NULL && i < 50) {
      input_commands[i++] = cmd;
      cmd = strtok(NULL, "|");
    }

    // extract args from each command
    char* args[100];
    i = 0;

    // currently parse first command only
    // Yusef wael will hanle pipes
    char* token = strtok(input_commands[0], " \t");
    while (token != NULL && i <  99) {
      args[i++] = token;
      token = strtok(NULL, " \t"); // parse the next word
    }

    if (i == 0) {
      continue;
    }

    bool background = false;
    if (strcmp(args[i - 1], "&") == 0) {
      background = true;
      i--;
    }

    args[i] = NULL; // to find execvp 

    // to detect if command found
    bool found_flag = 0;
    
    // loop through commands list
    for(int i = 0; commands[i] != NULL; i++) {
      if (args[0] != NULL && strcmp(commands[i]->name, args[0]) == 0) {
        commands[i]->run(commands[i], args, background);

        found_flag = true;
        break;
      }
    }
    
    // other command using fork and exec
    if(!found_flag) {
      // try to run unbuilt command
      Command* generic_cmd = unbuiltin_command(args[0]);
      generic_cmd->run(generic_cmd, args, background);
      free(generic_cmd);

    }    
  }

  return 0;
}
