- Built-in commands ALWAYS run in foreground


### Test cases
#### Build in commands without fork() nor exec()
##### single direct command
```bash
$ exit
yahya@Yahia-Mahmoud:/mnt/d/University/Third Year/Second semester/OS/project/project$ 
```
```bash
$ cd ../ # step back
$ cd project # step forward
$ cd gg # non existing dir
cd: No such file or directory
$ cd # without any arguments
cd: at least one argument
```
```bash
$ history
1  ls
2  cd ../
3  cd project
4  cd gg
5  cd
6  history
```
Note: `history` adds the command to the history before execution, includes also commands that revealed an error (as it is added before execution we do not know if it will produce an error).
```bash
$ pwd
/mnt/d/University/Third Year/Second semester/OS/project/project
```
##### pwd and history with input/output redirections
`pwd` and `history` can have input/output redirections, but `cd` and `exit` do not.
```bash
$ pwd > pwd.txt

# Show if the content is added correctly to the file
$ cat pwd.txt
/mnt/d/University/Third Year/Second semester/OS/project/project
```
```bash
$ history > hist.txt

# Show if the content is added correctly to the file
$ cat < hist.txt
1  ls
2  cd ../
3  cd project
4  cd gg
5  cd
6  history
7  pwd
8  clear
9  pwd > pwd.txt
10  cat pwd.txt
11  cat < pwd.txt
12  history > hist.txt
```

#### Other commands with fork() nor exec()
##### single command
```bash
$ ls -la
total 3944
drwxrwxrwx 1 yahya yahya    4096 Apr 17 09:18  .
drwxrwxrwx 1 yahya yahya    4096 Apr 17 06:44  ..
drwxrwxrwx 1 yahya yahya    4096 Apr 17 08:57  .vscode
-rwxrwxrwx 1 yahya yahya 4037806 Apr 17 06:44 'OS - Final Project.pdf'
drwxrwxrwx 1 yahya yahya    4096 Apr 17 08:49  codecrafters-shell-c
drwxrwxrwx 1 yahya yahya    4096 Apr 19 08:29  project
```

##### with input/output redirections
```bash
$ ls -la > ls.txt # output redirection
$ cat < ls.txt # input redirection
total 3944
drwxrwxrwx 1 yahya yahya    4096 Apr 19 08:34 .
drwxrwxrwx 1 yahya yahya    4096 Apr 17 06:44 ..
drwxrwxrwx 1 yahya yahya    4096 Apr 17 08:57 .vscode
-rwxrwxrwx 1 yahya yahya 4037806 Apr 17 06:44 OS - Final Project.pdf
drwxrwxrwx 1 yahya yahya    4096 Apr 17 08:49 codecrafters-shell-c
-rwxrwxrwx 1 yahya yahya       0 Apr 19 08:34 ls.txt
drwxrwxrwx 1 yahya yahya    4096 Apr 19 08:29 project
```

#### foreground/background execution
Note: It is unlogical to have background execution for `exit` and `cd`.
```bash
$ sleep 5 &
[PID 2564]
$ pwd &
[PID 2567]
$ /mnt/d/University/Third Year/Second semester/OS/project
```

#### Pipes
##### Two stage
```bash
$ cat /etc/passwd | grep root
root:x:0:0:root:/root:/bin/bash
```
##### Multi stage
```bash
$ ls | grep \.c$      
builtin.c
myShell.c
unbuiltin_command.c
```
Note that: our shell does not require `''`, should be avoided.
##### With built in
```bash
$ history | grep ls
1  ls | grep '\.c$'
2  ls | grep \.c$
3  history | grep ls
```
Note: It not logicall to hold `exit` or `cd` in pipes, as each command in the pipe are forked then executed separately, so if `exit` called it will end the process itself not the pipe as a whole, if `cd` is called then this will change the directory on the forked process not in the parent process, so no effect at all, you will be on the same position.

#### long data pipeline
```bash
$ cat Makefile | grep . | sort | uniq
        gcc myShell.c builtin.c
        rm -f sysinfo
all:
clean:
```

#### redirection with pipes
##### input + pipe
```bash
grep main < myShell.c | wc -l
1
```

##### output + pipe
```bash
$ ls | grep \.h$ > headers.txt
$ cat headers.txt
Command.h
ExecutionContext.h
builtin.h
```

##### Input + output + pipe
```bash
$ grep include < myShell.c | sort > includes_sorted.txt
$ cat includes_sorted.txt
#include "ExecutionContext.h"
#include "builtin.h"
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
```

#### Edge cases
Should be in the same directory
```bash
$ ls
'OS - Final Project.pdf'   codecrafters-shell-c   ls.txt   project
$ cd .. | ls
'OS - Final Project.pdf'   codecrafters-shell-c   ls.txt   project
```

Should not exit the shell
```bash
$ exit | pwd
/mnt/d/University/Third Year/Second semester/OS/project
$ # our shell is not terminated
```

### Signal handling
```bash
$ sleep 10 | sleep 10 | sleep 10
^C$ # The foreground ends not the shell
```

```bash
$ sleep 100
^Z^Z^Z
^Z^Z^Z
^C
yahya@Yahia-Mahmoud:/mnt/d/University/Third Year/Second semester/OS/project/project$
```

