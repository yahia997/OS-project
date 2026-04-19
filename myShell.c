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

void execute_single_command(char** args, ExecutionContext* ctx, char*** history, int history_cnt) { 
  pid_t pid = fork(); 
  if (pid < 0) { 
    perror("fork failed"); 
    return; 
  } 
  if(pid == 0) { 
    // Child process
    signal(SIGTSTP, SIG_DFL);
    signal(SIGINT, SIG_DFL);  // Child should handle Ctrl+C normally
    
    // Put child in its own process group
    setpgid(0, 0);

    // handle input redirection 
    // read data from file 
    if(ctx->input_file) { 
      int fd = open(ctx->input_file, O_RDONLY); 
      if (fd < 0) { 
        perror("input file"); 
        exit(1); 
      } 
      dup2(fd, STDIN_FILENO); 
      close(fd); 
    } 
    
    if (ctx->output_file) { 
      int fd = open(ctx->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644); 
      if (fd < 0) { 
        perror("output file"); 
        exit(1); 
      } 
      dup2(fd, STDOUT_FILENO); 
      close(fd); 
    }
    
    // use buildin implementation
    if (builtin_pwd_history(args, history, history_cnt)) exit(0);

    // another command than pwd and history
    execvp(args[0], args); 
    perror(args[0]); 
    exit(EXIT_FAILURE); 
  } else { 
    // parent process
    if (!ctx->is_background) {
      // Give terminal control to the child's process group for foreground execution
      tcsetpgrp(STDIN_FILENO, pid);
    }
    
    if (ctx->is_background) { // background option 
      printf("[PID %d]\n", pid); 
      // print process id 
    } else { 
      // foreground exec option 
      int status;
      waitpid(pid, &status, 0);
      
      // Take back terminal control
      tcsetpgrp(STDIN_FILENO, getpgid(0));
    } 
  } 
}

void execute_pipeline(char*** args, int n, ExecutionContext* ctx, char*** history, int history_cnt) {
  int in_fd = -1;
  pid_t pids[50];
  int valid_pids = 0;
  pid_t pgid = 0;  // Process group ID for the pipeline

  for (int i = 0; i < n; i++) {
    // CRITICAL: Robust NULL Check
    // If the command pointer is NULL, or the first argument is NULL, skip this segment.
    if (args[i] == NULL || args[i][0] == NULL) {
        continue; 
    }

    int pipefd[2];
    // Check if there is a VALID next command before piping
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
      // Child process logic
      signal(SIGTSTP, SIG_DFL);
      signal(SIGINT, SIG_DFL);  // Child should handle Ctrl+C normally
      
      // Put all children in the same process group
      if (pgid == 0) {
        // First child: create new process group
        setpgid(0, 0);
      } else {
        // Subsequent children: join the first child's group
        setpgid(0, pgid);
      }
      
      if (in_fd != -1) { 
        dup2(in_fd, STDIN_FILENO);
        close(in_fd);
      }

      if (has_next) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
      }

      // First command file redirection
      if (i == 0 && ctx[i].input_file) {
        int fd = open(ctx[i].input_file, O_RDONLY);
        if (fd < 0) { perror("input file"); exit(1); }
        dup2(fd, STDIN_FILENO);
        close(fd);
      }

      // Last command file redirection
      if (i == n - 1 && ctx[i].output_file) {
        int fd = open(ctx[i].output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) { perror("output file"); exit(1); }
        dup2(fd, STDOUT_FILENO);
        close(fd);
      }

      if (builtin_cd_exit(args[i])) exit(0);
      if (builtin_pwd_history(args[i], history, history_cnt)) exit(0);

      execvp(args[i][0], args[i]);
      perror("execvp");
      exit(1);

    } else {
      // Parent process logic
      // Set up process group
      if (pgid == 0) {
        pgid = pid;  // First child's PID becomes the group ID
        setpgid(pid, pgid);
      } else {
        setpgid(pid, pgid);
      }
      
      pids[valid_pids++] = pid;
      
      if (in_fd != -1) {
        close(in_fd);
      }
  
      if (has_next) {
        close(pipefd[1]);
        in_fd = pipefd[0];
      }
    }
  }

  // CRITICAL: Close the very last pipe's read-end in the parent
  if (in_fd != -1) {
      close(in_fd);
  }

  if (ctx[0].is_background) {
    printf("[pipeline running in background]\n");
    return;
  }

  // Give terminal control to the pipeline's process group for foreground execution
  if (pgid > 0) {
    tcsetpgrp(STDIN_FILENO, pgid);
  }

  // Wait for all processes in the pipeline
  for (int i = 0; i < valid_pids; i++) {
    int status;
    waitpid(pids[i], &status, 0);
  }
  
  // Take back terminal control
  tcsetpgrp(STDIN_FILENO, getpgid(0));
}

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
            if (len == 1) {
                // Skip this token entirely (don't copy it)
                continue;
            } else {
                args[j][len - 1] = '\0'; // Fixes the "5&" issue
            }
        }

        if (strcmp(args[j], "<") == 0) {
            ctx->input_file = args[j + 1];
            j++; // Skip the next argument (filename)
            continue;

        } else if (strcmp(args[j], ">") == 0) {
            ctx->output_file = args[j + 1];
            j++; // Skip the next argument (filename)
            continue;

        } else if (strcmp(args[j], "&") == 0) {
            ctx->is_background = 1;
            continue; // Skip this token
        }
        
        // Keep this argument
        args[write_idx++] = args[j];
    }
    
    // Null-terminate at the new end
    args[write_idx] = NULL;
}

void add_to_history(char*** history, int* history_cnt, char* command) {
    // Check if we've reached max history size
    if (*history_cnt >= 1000) {
        return; // Or implement circular buffer
    }
    
    // Allocate memory for the new command
    history[*history_cnt] = (char**)malloc(2 * sizeof(char*));
    
    if (history[*history_cnt] == NULL) {
        perror("malloc failed for history entry");
        return;
    }
    
    // Store a copy of the command
    history[*history_cnt][0] = strdup(command);
    
    if (history[*history_cnt][0] == NULL) {
        perror("strdup failed");
        free(history[*history_cnt]);
        return;
    }
    
    history[*history_cnt][1] = NULL;
    
    (*history_cnt)++;
}

int main(int argc, char *argv[]) {
  // to show output immediately
  setbuf(stdout, NULL);

  // Initialize history
  char*** history = (char***)malloc(1000 * sizeof(char**));
  int history_cnt = 0;

  // Signal handling for the shell
  signal(SIGINT, SIG_IGN);   // Ignore Ctrl+C in the shell
  signal(SIGTSTP, SIG_IGN);  // Ignore Ctrl+Z in the shell
  signal(SIGTTOU, SIG_IGN); // Ignore "Background Write to Terminal"
  signal(SIGTTIN, SIG_IGN); // Ignore "Background Read from Terminal"

  // REPL (Read-Eval-Print Loop)
  while(1) {
    
    // display prompt
    printf("$ ");
    
    // take user commands
    char command[1024];
    if (fgets(command, sizeof(command), stdin) == NULL) {
      // EOF or error - exit gracefully
      printf("\n");
      break;
    }
    
    // so remove trailing new line
    command[strcspn(command, "\n")] = '\0';

    // Skip empty commands
    if (strlen(command) == 0) continue;

    // Make a copy for parsing since strtok modifies the string
    char command_copy[1024];
    strcpy(command_copy, command);

    // Add ORIGINAL command to history (not the copy that will be modified)
    add_to_history(history, &history_cnt, command);

    // list to handle pipes in future
    char* input_commands[50];
    int cmd_count = 0;

    char* cmd = strtok(command_copy, "|");
    while (cmd != NULL && cmd_count < 50) {
      input_commands[cmd_count++] = cmd;
      cmd = strtok(NULL, "|");
    }

    // extract args from each command
    char* args_list[50][100];
    char** args_ptrs[50];
    ExecutionContext ctx_list[50];

    for (int i = 0; i < cmd_count; i++) {
      args_ptrs[i] = args_list[i];
      parse_command(input_commands[i], args_list[i], &ctx_list[i]);
    }

    // Validate pipeline: if we have multiple segments, check that each has a valid command
    if (cmd_count > 1) {
      bool incomplete_pipe = false;
      
      // Check if first segment is empty
      if (args_ptrs[0][0] == NULL) {
        fprintf(stderr, "Error: missing command before pipe\n");
        continue;
      }
      
      // Check if last segment is empty
      if (args_ptrs[cmd_count - 1][0] == NULL) {
        fprintf(stderr, "Error: missing command after pipe\n");
        continue;
      }
      
      // Could also check middle segments, but those are less common
    }

    // built in exit command and cd
    // moved far away from any fork
    if (cmd_count == 1 && builtin_cd_exit(args_list[0])) continue;

    if(cmd_count == 1) {
      execute_single_command(args_ptrs[0], &ctx_list[0], history, history_cnt);
      continue;
    }

    // If any command in the pipeline has the background flag, 
    // the whole pipeline should be backgrounded.
    int pipeline_bg = 0;
    for(int i = 0; i < cmd_count; i++) {
        if(ctx_list[i].is_background) pipeline_bg = 1;
    }
    // Set the first one so execute_pipeline sees it
    ctx_list[0].is_background = pipeline_bg;
    
    // other command using fork and exec
    // try to run unbuilt command
    execute_pipeline(args_ptrs, cmd_count, ctx_list, history, history_cnt);
  }

  // Free history before exit
  for (int i = 0; i < history_cnt; i++) {
    free(history[i][0]);
    free(history[i]);
  }
  free(history);

  return 0;
}