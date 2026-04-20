CC = gcc
CFLAGS = -Wall -Wextra -Werror
TARGET = myShell

all:
	$(CC) myShell.c builtin.c add_to_history.c execute_pipeline.c execute_single_command.c parse_command.c -o $(TARGET) $(CFLAGS)