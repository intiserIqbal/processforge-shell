# ProcessForge Shell — Development Log

## Day 6: Full Job Control – Foreground, Background, and Signal Delivery

### Objective
Extend the shell with complete job control semantics: bring stopped jobs to the foreground (`fg`), resume jobs in the background (`bg`), and send arbitrary signals to entire process groups (`kill`). This turns the job table from a passive tracker into an active controller.

---

### 1. New Built‑ins for Job Control
**Files:** `src/builtins.c`, `include/shell.h`

- Added `builtin_fg()`, `builtin_bg()`, and `builtin_kill()` to `builtins.c`.
- **fg**
  - Takes a job ID (e.g., `%1`).
  - Gives terminal control to the job’s process group with `tcsetpgrp()`.
  - Sends `SIGCONT` to the entire process group (`kill(-pgid, SIGCONT)`).
  - Waits for the job to stop or exit using a loop with `WUNTRACED | WCONTINUED`.
  - Restores terminal to the shell when done.
- **bg**
  - Similar, but does not take the terminal; simply sends `SIGCONT` and marks the job as `RUNNING`.
- **kill**
  - Accepts an optional signal number (e.g., `kill -9 %1`) and sends that signal to the job’s process group.
- Updated `shell.h` with prototypes for all three new built‑ins.

---

### 2. Pipeline Command String Fix
**Files:** `src/executor.c`, `src/main.c`, `include/shell.h`

- Previously, pipelines were added to the job table with the hardcoded string `"pipeline"`.
- Changed `execute_pipeline()` signature to accept `const char *original_command`.
- Both background and foreground job entries now use `original_command`.
- Updated `main.c` to pass `original_command` when calling `execute_pipeline()`.

---

### 3. Correct Waiting Loop for Foreground Pipelines
**Files:** `src/executor.c`

- Old loop failed when `SIGCHLD` handler reaped all children before `waitpid()`.
- Replaced with a cleaner loop that:
  - Waits until no children remain or a stop occurs.
  - Ignores `ECHILD` gracefully.
- Pipelines now terminate cleanly without spurious errors.

---

### 4. Terminal Control in `builtin_fg`
**Files:** `src/builtins.c`

- Initial implementation returned prompt immediately.
- Fixed by:
  - Marking job as `JOB_RUNNING` before wait loop.
  - Using `waitpid(-pgid, &status, WUNTRACED | WCONTINUED)` in a `do-while`.
  - Loop continues while job is merely continued (`WIFCONTINUED`).
  - Stops only when job halts or exits.
- Restores terminal ownership to shell after job finishes/stops.

---

### 5. Compilation and Header Cleanup
**Files:** `src/builtins.c`

- Added missing includes: `<sys/wait.h>` and `<sys/types.h>`.
- Defined `_POSIX_C_SOURCE 200809L` to guarantee `WCONTINUED` and `WIFCONTINUED`.
- Resolved linker errors by ensuring original built‑ins (`cd`, `help`, `exit`, `jobs`) remain present.

---

### 6. Verification & Testing
Manual tests performed:

| Command                  | Expected Behavior                          | Result |
|---------------------------|--------------------------------------------|--------|
| `sleep 10 → ^Z → jobs`   | Job shows as Stopped                       | ✅     |
| `fg %1`                  | Waits 10 seconds, then prompt returns      | ✅     |
| `sleep 30 & → kill %1`   | Background job terminates, removed from list | ✅   |
| `yes \| head -n5 & → jobs` | Pipeline appears with correct command string | ✅   |
| `yes \| head -n5`        | Prints exactly five `y`s, no errors        | ✅     |

All pipelines, redirections, and built‑ins from previous days remain functional.

---

## Day 6 Achievements
- Full job control: `fg`, `bg`, `kill` with correct process‑group semantics.
- Pipelines properly named in job table.
- No more `waitpid: No child processes` errors.
- Foreground resume (`fg`) correctly blocks shell until job stops/finishes.
- Terminal ownership consistently transferred and restored.
- Code compiles cleanly under `-Wall -Wextra -Werror`.

---

## Next Steps (Beyond Day 6)
- Add support for `wait` built‑in to wait for any child process.
- Implement environment variable expansion (`$HOME`, `$PATH`).
- Introduce command history using `readline` or a custom ring buffer.
- Begin scheduler integration: process priorities, CPU time accounting.
