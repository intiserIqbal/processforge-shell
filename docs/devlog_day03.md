# ProcessForge Shell — Development Log

## Day 3: Robust Redirection, Parser Fixes & Input Handling

### Objective

The goal for Day 3 was to make input/output redirection robust, improve the parser to handle whitespace, newlines, and redirection operators correctly, and make the shell more reliable for both interactive and script-driven use.

---

## 1. Redirection Parsing

**File:**  
parser.c

- The parser now detects `<`, `>`, and `>>` and sets the appropriate fields in the `Command` struct.
- Redirection operators and filenames are removed from the argument list.
- This enables commands like `echo hello > file.txt` and `cat < file.txt`.

---

## 2. Whitespace & Newline Handling

**File:**  
parser.c, main.c

- The parser splits tokens on all whitespace and newlines (`strtok(input, " \t\r\n")`).
- Input is processed line-by-line via fgets, ensuring compatibility with both interactive and piped/script input.
- This fixes issues where commands like `ls | wc -l\nexit` were misinterpreted.

---

## 3. Single-Stage Pipeline Limitation & Validation

**File:**  
parser.c

- The shell now explicitly checks for multiple pipes (`|`) and rejects input with more than one, printing an error.
- This prevents misparsing of commands like `ls | grep .c | wc -l`.

---

## 4. Redirection in Pipeline Execution

**File:**  
executor.c

- The pipeline executor now applies input redirection for the first command and output redirection for the second command in a pipeline.
- This enables commands like `cat < input.txt | grep hello > out.txt`.

---

## 5. Builtin Command Handling

**File:**  
executor.c

- The executor now returns immediately after handling the `exit` builtin, preventing unwanted fork and improving shell stability.

---

## 6. Safety and Argument Cleanup

**File:**  
executor.c

- Before every execvp, the code checks for a valid command and strips any trailing newlines from arguments.
- This prevents errors like `wc: invalid option -- '\'` when using script input.

---

## 7. Testing Performed

**Manual and script-based tests:**

- `echo hello > file.txt`
- `cat < file.txt`
- `cat < input.txt | grep hello > out.txt`
- `ls | wc -l`
- `printf "ls | wc -l\nexit\n" | ./build/processforge`
- `ls | grep .c | wc -l` (now correctly rejected)

**Results:**

- Redirection works for both standalone and pipeline commands.
- Input cleaning and parsing are robust.
- Single-stage pipelines are functional, but multi-stage pipelines are explicitly rejected.

---

## 8. Observed Issues

- Multi-stage pipelines (`cmd1 | cmd2 | cmd3`) are not supported and are now explicitly rejected.
- Some edge cases with script input were fixed by line-by-line processing and argument cleanup.
- The parser and executor are now more robust, but the shell is still limited to single-stage pipelines.

---

## Day 3 Achievements

- Robust redirection parsing and execution
- Improved whitespace and newline handling
- Explicit pipeline limitation and error reporting
- Stable shell behavior for common Unix command patterns and script input
- Foundation laid for future multi-stage pipeline support

---

**Next Steps:**  
- Implement multi-stage pipelines
- Add job control and scripting features
- Expand test coverage

---