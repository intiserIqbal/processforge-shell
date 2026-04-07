# ProcessForge Shell — Development Log

## Day 2: Inter-Process Communication with Pipes

### Objective

The goal for Day 2 was to implement **pipeline support**, allowing the shell to execute commands connected via a Unix pipe (`|`).

Example command:

```
ls | grep txt
```

Pipelines allow the **output of one process to become the input of another**, enabling powerful command composition in Unix systems.

This feature introduces **inter-process communication (IPC)** using the POSIX `pipe()` system call.

---

# 1. Pipeline Concept

In Unix shells, a pipeline connects two processes through a **kernel-managed buffer**.

Example:

```
ls | grep txt
```

Execution model:

```
ls (stdout) → pipe → grep (stdin)
```

Process structure:

```
processforge
      │
      ├─ fork() → child 1 → exec(ls)
      │             stdout → pipe write end
      │
      └─ fork() → child 2 → exec(grep)
                    stdin  ← pipe read end
```

This allows data to flow **directly between processes without temporary files**.

---

# 2. Parser Extension for Pipe Detection

File:

```
src/parser.c
```

The parser was extended to detect the pipe operator (`|`) and split the command into two parts.

Example input:

```
ls | grep txt
```

Parsed into:

```
Command 1:
args = ["ls"]

Command 2:
args = ["grep", "txt"]
```

A new structure was introduced to represent a pipeline:

```c
typedef struct {
    Command left;
    Command right;
    int is_pipe;
} Pipeline;
```

Meaning:

```
left  → command before |
right → command after |
```

This allows the executor to determine whether to run a **single command or a pipeline**.

---

# 3. Pipe Creation Using `pipe()`

File:

```
src/executor.c
```

A pipe was created using the POSIX system call:

```
pipe(pipefd)
```

This creates two file descriptors:

| Descriptor | Purpose           |
| ---------- | ----------------- |
| pipefd[0]  | read end of pipe  |
| pipefd[1]  | write end of pipe |

Data written to the write end can be read from the read end.

---

# 4. Redirecting File Descriptors with `dup2()`

To connect the two processes, standard input and output were redirected.

### First Child Process (Producer)

```
dup2(pipefd[1], STDOUT_FILENO)
```

Effect:

```
stdout → pipe write end
```

Any output produced by the command (e.g., `ls`) is written into the pipe.

---

### Second Child Process (Consumer)

```
dup2(pipefd[0], STDIN_FILENO)
```

Effect:

```
stdin ← pipe read end
```

The second command (e.g., `grep`) reads its input from the pipe.

---

# 5. Proper Pipe Closure

Correct pipe handling requires closing unused pipe ends.

Rules applied:

| Process           | Pipe Ends Closed |
| ----------------- | ---------------- |
| Producer (`ls`)   | read end         |
| Consumer (`grep`) | write end        |
| Parent shell      | both ends        |

Closing unused descriptors prevents:

```
deadlocks
resource leaks
EOF detection failures
```

---

# 6. Pipeline Execution Algorithm

The shell now executes pipelines using the following steps:

```
1. Create pipe
2. fork() → first child
3. Redirect stdout → pipe write end
4. exec() command 1
5. fork() → second child
6. Redirect stdin ← pipe read end
7. exec() command 2
8. Parent closes pipe
9. Parent waits for both children
```

This ensures both commands run concurrently.

---

# 7. Example Execution

User command:

```
processforge> ls | grep c
```

Internal execution:

```
fork → child1 → exec(ls)
fork → child2 → exec(grep)

ls output → pipe → grep input
```

Example result:

```
src
scripts
```

---

# 8. System Calls Introduced

Day 2 introduced several important Unix system calls:

| System Call | Purpose                      |
| ----------- | ---------------------------- |
| `pipe()`    | create communication channel |
| `dup2()`    | redirect file descriptors    |
| `fork()`    | spawn child processes        |
| `execvp()`  | execute program              |
| `close()`   | release unused pipe ends     |
| `waitpid()` | wait for child processes     |

These system calls form the foundation of **process communication in Unix systems**.

---

# 9. Testing Performed

Manual tests were performed to verify pipeline behavior.

### Test 1

```
ls | grep c
```

Expected behavior:

Filter directory listing.

---

### Test 2

```
cat src/main.c | wc -l
```

Expected behavior:

Count number of lines in the file.

---

### Test 3

```
ps aux | grep bash
```

Expected behavior:

Find running Bash processes.

---

# 10. Observed Behavior

Successful results confirmed:

```
✔ data flows correctly through pipes
✔ commands execute concurrently
✔ standard input/output redirection works
✔ no pipe deadlocks occurred
```

The pipeline functionality behaves similarly to traditional Unix shells.

---

# Known Limitations (Day 2)

The current implementation supports **only a single pipe**.

Supported:

```
command1 | command2
```

Not yet supported:

```
command1 | command2 | command3
```

Multi-stage pipelines will be implemented in future updates.

---

# Day 2 Achievements

New functionality added:

```
- pipeline detection in parser
- command splitting at '|'
- pipe-based IPC
- file descriptor redirection
- concurrent execution of pipeline processes
- proper pipe resource management
```

With this feature, **ProcessForge now supports Unix-style pipelines**, significantly expanding the shell’s capabilities.
