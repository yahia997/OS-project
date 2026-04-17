#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Command.h"
#include <stdbool.h>

// define built in commands
Command* exit_command();
Command* pwd_command();
Command* unbuiltin_command();

int main(int argc, char *argv[]) {
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
    
    // display prompt
    printf("$ ");
    
    // take user commands
    char command[1024];
    fgets(command, sizeof(command), stdin); // includes new line
    
    // so remove trailing new line
    command[strcspn(command, "\n")] = '\0';

    // extract args
    char* args[100];
    int i = 0;

    char* token = strtok(command, " ");
    while (token != NULL && i <  99) {
      args[i++] = token;
      token = strtok(NULL, " "); // parse the next word
    }
    args[i] = NULL; // to find execvp 

    // to detect if command found
    bool found_flag = 0;
    
    // loop through commands list
    for(int i = 0; commands[i] != NULL; i++) {
      if (args[0] != NULL && strcmp(commands[i]->name, args[0]) == 0) {
        commands[i]->run(commands[i], args);

        found_flag = true;
      }
    }
    
    // other command using fork and exec
    if(!found_flag) {
      // try to run unbuilt command
      Command* generic_cmd = unbuiltin_command(args[0]);
      generic_cmd->run(generic_cmd, args);

    }    
  }

  return 0;
}
