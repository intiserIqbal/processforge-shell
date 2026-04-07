# ProcessForge Shell — Development Log

## Day 1: Core Shell Framework

### Project

**ProcessForge Shell** – A lightweight Unix-like shell implemented in **C** to study process management, system calls, and operating system concepts.

The shell interacts with the OS using key POSIX system calls such as:

- `fork()`
- `execvp()`
- `waitpid()`
- `getcwd()`

These are foundational APIs used in Unix-like systems including Linux and shells like Bash.

---

## 1. Repository Architecture Designed

A modular structure was created to separate responsibilities.

```
processforge-shell/
│
├── src/        → implementation files
├── include/    → header definitions
├── tests/      → shell behavior tests
├── docs/       → architecture + documentation
├── scripts/    → installer
├── build/      → compiled binaries
```

### Reasoning

This separation mimics real production software layout:

| Folder  | Purpose                                   |
| ------- | ----------------------------------------- |
| src     | core shell logic                          |
| include | shared structures and function prototypes |
| docs    | project documentation                     |
| tests   | behavior verification                     |
| scripts | automation (installation)                 |

This structure improves maintainability and research readability.

---

## 2. Core Shell Loop Implemented

File:

```
src/main.c
```

The shell follows the classic REPL architecture:

```
Read → Parse → Execute → Loop
```

### Execution Flow

```
User input
   ↓
parse_input()
   ↓
Command structure
   ↓
execute_command()
   ↓
fork()
   ↓
execvp()
```

---

## 3. Command Parsing System

Implemented in:

```
src/parser.c
```

The parser converts raw text input into structured commands.

Example:

```
sleep 5 &
```

becomes:

```
args[0] = "sleep"
args[1] = "5"
background = 1
```

### Design

A `Command` structure was introduced:

```c
typedef struct {
    char *args[MAX_ARGS];
    int background;
} Command;
```

This allows easy extension later for:

- pipes
- redirection
- job control

---

## 4. Process Execution Engine

Implemented in:

```
src/executor.c
```

The shell launches programs using the standard Unix process model:

```
fork() → child process created
execvp() → replace child with program
waitpid() → parent waits
```

Example flow:

```
processforge> ls
```

Execution:

```
shell
 ├─fork()
 │   └─ child → execvp("ls")
 └─ parent → waitpid()
```

This mirrors the design used by real shells.

---

## 5. Built-in Command System

Implemented in:

```
src/builtins.c
```

Some commands must run inside the shell process rather than via `exec`.

Implemented built-ins:

```
cd
help
exit
```

Example:

```
processforge> cd build
```

This works by calling:

```
chdir()
```

inside the shell process.

---

## 6. Signal Handling

Implemented in:

```
src/signals.c
```

The shell handles Ctrl+C gracefully using:

```
SIGINT
```

Instead of terminating the shell, the signal prints a new prompt.

This improves usability and mimics real shell behavior.

---

## 7. Background Process Support

Basic support added for `&`.

Example:

```
processforge> sleep 5 &
```

Behavior:

```
fork()
child → runs command
parent → does NOT wait
```

Output example:

```
[background pid 6800]
```

This demonstrates asynchronous process execution.

---

## 8. Dynamic Shell Prompt

Today you improved the prompt to display the current working directory using:

```
getcwd()
```

Two styles implemented:

### Full path

```
processforge:/home/user/project$
```

### Clean directory name

```
processforge:project$
```

Example session:

```
processforge:mini-shell$ cd build
processforge:build$ cd ..
processforge:mini-shell$
```

This improves usability and mirrors real shell UX.

---

## 9. Build System

Created a Makefile for compilation.

Command:

```
make
```

Produces:

```
build/processforge
```

This allows reproducible builds across systems.

---

## 10. Testing Performed

Basic manual tests were executed.

Example session:

```
processforge> ls
processforge> cd build
processforge> pwd
processforge> sleep 5 &
processforge> help
```

Observed behaviors:

- command execution
- builtins working
- background processes
- working directory prompt

---

## Known Limitations (Day 1)

The shell currently does not yet support:

```
pipes
redirection
job control
command history
autocomplete
```

Example unsupported command:

```
ls | grep txt
```

These will be implemented in later stages.

---

## Day 1 Achievements

Core shell features implemented:

```
- interactive shell loop
- command parsing
- process creation (fork)
- program execution (execvp)
- waiting (waitpid)
- builtins
- background jobs
- signal handling
- dynamic prompt
- modular architecture
- Makefile build system
```