// abstract class to create other commands

#include <stdbool.h>

typedef struct Command  {
  // command
  char* name; 
  int max_args;
    
  // abstract method will be overriden on other commands
  void (*run)(struct Command* self, char** args, bool background);
  void (*help)(struct Command* self);
} Command;
