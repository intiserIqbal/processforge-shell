# ProcessForge Shell

`https://img.shields.io/github/repo-size/intiserIqbal/processforge-shell`
`https://img.shields.io/github/last-commit/intiserIqbal/processforge-shell`
`https://img.shields.io/github/license/intiserIqbal/processforge-shell`

---

## Introduction & Motivation

**ProcessForge Shell** is a research‑driven, Unix‑like command interpreter implemented in C. It is designed to expose core shell mechanisms such as parsing, process creation, pipelines, redirection, and signal handling. Unlike typical student shells, ProcessForge emphasizes modularity and extensibility, making it suitable for experimentation with advanced OS features such as job control and custom scheduling.

---

## Project Structure

```
mini-shell/
├── src/
│   ├── main.c               # Entry point and shell loop (dynamic prompt, signal handling)
│   ├── parser.c             # Command parsing and pipeline construction
│   ├── executor.c           # Execution, redirection, globbing, and pipelines
│   └── builtins.c           # Built-in commands
├── include/
│   └── shell.h              # Shared command/pipeline definitions
├── docs/
│   ├── architecture.md      # Architecture documentation
│   ├── demo.md              # Demo steps and usage plan
│   └── devlogs/             # Development logs
├── scripts/
│   └── install.sh           # Optional install helper
├── tests/
│   ├── test_builtins.sh     # Builtins test
│   ├── test_pipes.sh        # Pipeline test
│   ├── test_redirection.sh  # Redirection test
│   ├── expected/            # Expected outputs
│   ├── fixtures/            # Input fixtures
│   └── temp/                # Temp files (gitignored)
├── build/                   # Compiled binaries
├── Makefile
├── Dockerfile
├── LICENSE
└── README.md
```

---

## Current Implementation

- **Dynamic Prompt:** Interactive sessions show `processforge:/cwd>`; scripted tests suppress prompts for clean output.
- **Parser:** Handles pipes (`|`), redirection (`<`, `>`, `>>`), background execution (`&`), and whitespace/newline cleanup.
- **Executor:** 
  - Builtins (`cd`, `help`, `exit`) handled directly.
  - External commands launched with `fork()` + `execvp()`.
  - Multi‑stage pipelines supported with `pipe()` + `dup2()`.
  - Redirection applied per command.
  - Globbing expands wildcards before execution.
- **Signal Handling:** Custom SIGINT handler restores prompt gracefully.
- **Tests:** Expanded to cover builtins, multi‑stage pipelines, and redirection (append/input).

---

## Key Features

- **External Program Execution:** Launches binaries using `fork()` and `execvp()`.
- **Multi‑Stage Pipelines:** Arbitrary‑length pipelines supported (`cmd1 | cmd2 | cmd3 | ...`).
- **Input/Output Redirection:** Supports `<`, `>`, and `>>`.
- **Background Execution:** Handles `&` for background jobs.
- **Signal Handling:** Manages interrupts (Ctrl+C).
- **Built‑in Commands:** Includes `cd`, `help`, and `exit`.
- **Dynamic Prompt:** Shows current working directory in interactive mode.

---

## System Calls Used

| System Call | Purpose           |
| ----------- | ----------------- |
| fork()      | Create process    |
| execvp()    | Load program      |
| waitpid()   | Synchronize child |
| pipe()      | IPC (pipelines)   |
| dup2()      | Redirection       |
| open()      | Open files for redirection |
| chdir()     | Change directory  |
| getcwd()    | Current directory |
| sigaction() | Install SIGINT handler |

---

## Build & Run

```bash
make clean && make
./build/processforge
```

---

## Example Usage

```
processforge:/home/user/project> help
processforge:/home/user/project> ls | grep .c | sort | wc -l
processforge:/home/user/project> echo hello > out.txt
processforge:/home/user/project> cat < out.txt
processforge:/home/user/project> sleep 5 &
processforge:/home/user/project> exit
```

---

## Tests

```bash
./tests/test_builtins.sh
./tests/test_redirection.sh
./tests/test_pipes.sh
```

---

## Research Directions

ProcessForge is evolving into a research platform for studying OS concepts:

- **Job Control:** Implement `jobs`, `fg`, `bg`, and `kill` with a job table.
- **Scheduling Policies:** Add FCFS, SJF, and Round Robin schedulers for background jobs.
- **Instrumentation:** Collect metrics (latency, turnaround time, resource usage).
- **Prompt Extensions:** Show job count and scheduler policy in the prompt.
- **Reproducibility:** Document experiments in `docs/` and expand automated test coverage.

---

## License

Licensed under the **MIT License**. See `LICENSE` for details.
