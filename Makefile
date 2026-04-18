CC = gcc
CFLAGS = -Wall -Wextra -std=c11
TARGET = myShell
SOURCES = myShell.c pwd.c exit.c unbuiltin_command.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET) *.o
