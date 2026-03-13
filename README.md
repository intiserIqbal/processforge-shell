# ProcessForge Shell

![Repo Size](https://img.shields.io/github/repo-size/yourusername/processforge-shell)
![Last Commit](https://img.shields.io/github/last-commit/yourusername/processforge-shell)
![License](https://img.shields.io/github/license/yourusername/processforge-shell)


**ProcessForge Shell** is a minimal Unix-like command interpreter written in C that demonstrates core operating system concepts such as process creation, inter-process communication, and signal handling. The project is designed as a learning-focused systems programming exercise to explore how a shell interacts with the Linux kernel through low-level system calls.

Rather than relying on existing frameworks, this shell is built from scratch using fundamental Unix primitives like `fork()`, `execvp()`, `waitpid()`, `pipe()`, and `dup2()`. By implementing these mechanisms directly, the project provides practical insight into how real shells manage processes, execute programs, and coordinate communication between commands.

---

## Features

The shell aims to support core functionality found in traditional Unix shells:

* Execute external programs
* Process creation using `fork()`
* Program execution using `execvp()`
* Process synchronization using `waitpid()`
* Command pipelines using `pipe()`
* Input and output redirection (`>`, `<`)
* Background process execution (`&`)
* Basic signal handling (e.g., `Ctrl+C`)

These features demonstrate essential operating system mechanisms such as process lifecycle management and inter-process communication.

---

## Project Structure

```
processforge-shell
│
├── src/
│   ├── main.c
│   └── shell.c
│
├── include/
│   └── shell.h
│
├── docs/
│   └── architecture.md
│
├── build/
│
├── Makefile
├── README.md
├── LICENSE
└── .gitignore
```

* **src/** – Core implementation files
* **include/** – Header files defining interfaces
* **docs/** – Project documentation and architecture notes
* **build/** – Compiled binaries (ignored by Git)

---

## Build Instructions

### Prerequisites

Install the required development tools:

```bash
sudo apt update
sudo apt install build-essential make gdb valgrind
```

### Compile the project

From the project root directory:

```bash
make
```

This builds the shell executable in the `build/` directory.

### Run the shell

```bash
./build/myshell
```

---

## Example Usage

```
myshell> ls
myshell> pwd
myshell> cat file.txt
myshell> ls | grep txt
myshell> sleep 10 &
myshell> exit
```

---

## Learning Goals

This project explores several important systems programming topics:

* Linux process model
* System calls and kernel interaction
* Inter-process communication (IPC)
* File descriptor manipulation
* Signal handling
* Debugging with `gdb`
* Memory analysis with `valgrind`

The architecture is intentionally modular to mirror real shell implementations and encourage maintainable systems-level design.

---

## Future Improvements

Potential extensions include:

* Command history
* Job control
* Tab completion
* Advanced parsing and quoting support
* Performance analysis of process creation and pipes

---

## License

This project is licensed under the **MIT License**. See the `LICENSE` file for details.
