CC = gcc
CFLAGS = -Wall -Wextra -Werror
TARGET = myShell

all:
	$(CC) myShell.c builtin.c -o $(TARGET) $(CFLAGS)

clean:
	rm -f $(TARGET) *.txt