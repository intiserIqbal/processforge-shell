ProcessForge Shell Architecture

The shell is implemented as a small, modular C program.

Entry point:
- `src/main.c` starts the REPL, prints `processforge>`, reads input, and dispatches parsing and execution.
- It also installs the SIGINT handler using `sigaction`.

Parsing:
- `src/parser.c` parses the typed command line into a `Pipeline`.
- It splits commands by `|`.
- It handles arguments, input/output redirection (`<`, `>`, `>>`), and background execution (`&`).

Execution:
- `src/executor.c` executes parsed commands.
- Built-in commands are handled directly in the parent process.
- External commands are launched with `fork()` + `execvp()`.
- Redirection is applied with `open()` and `dup2()`.
- Pipelines are implemented with `pipe()` and multiple child processes.

Builtins:
- `src/builtins.c` implements `cd`, `help`, and `exit`.

Header:
- `include/shell.h` defines `Command`, `Pipeline`, and public function prototypes.

Architecture flow:

```
processforge shell
    |
    +--> main.c: prompt, signal setup, REPL loop
             |
             +--> parser.c: tokenize input, build Command/Pipeline
                      |
                      +--> executor.c: builtin dispatch, fork/exec, pipes, redirection
                               |
                               +--> builtins.c: cd/help/exit
                               +--> external commands via execvp()
```

Key components:
- `main.c`: shell loop, prompt, signal handling
- `parser.c`: input parsing and command construction
- `executor.c`: process creation, redirection, pipes
- `builtins.c`: internal command support
- `include/shell.h`: shared types and function declarations