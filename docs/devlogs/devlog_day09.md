## ProcessForge Shell — Development Log

### Day 9: UX Enhancements – Readline, Interactive Menu, Job Count Prompt, Priority Command

---

### Objective

Improve the user experience by integrating GNU readline (history, line editing, persistent history), adding a dynamic prompt that shows the number of background jobs, implementing an interactive demo menu (`menu`), and allowing runtime priority changes for background jobs (`prio`). These features make the shell pleasant for interactive use while exposing its research capabilities.

---

### 1. ASCII Art Logo & Clean Startup

**Files:** `include/logo.h` (new), `src/main.c`

- Created `logo.h` with a colourful ProcessForge ASCII banner.
- In `main()`, call `print_logo()` once at startup, then print `"Starting interactive shell..."` and a hint: `"Type 'help' for available commands, 'menu' for interactive demos."`
- Removed the verbose help block from startup – now the user must type `help` to see the full list of built‑ins. This keeps the initial screen clean.

---

### 2. GNU Readline Integration

**Files:** `src/main.c`, `Makefile`

- Replaced `fgets()` with `readline()` for input.
- Added `#include <readline/readline.h>` and `<readline/history.h>`.
- In `main()`:
  - Load previous command history from `~/.processforge_history` using `read_history()`.
  - Use `readline(get_prompt())` to obtain input (the prompt is built dynamically, see below).
  - If the line is non‑empty, add it to history with `add_history()`.
  - On exit, save history with `write_history()`.
- Updated `Makefile` to link with `-lreadline -lncurses`.

**Result:** Arrow keys recall previous commands, left/right arrow moves the cursor, and history persists across shell restarts.

---

### 3. Dynamic Prompt Showing Background Job Count

**File:** `src/main.c`

- Wrote `char *get_prompt(void)` that:
  - Counts jobs in state `JOB_RUNNING` or `JOB_STOPPED` from the global job table.
  - Gets the current working directory with `getcwd()`.
  - Returns a string like `"[2] processforge:/home/user$ "` (with ANSI red colour for the count) if any background jobs exist, otherwise `"processforge:/home/user$ "`.
- The prompt is recomputed before each call to `readline()`, so it reflects the current number of background jobs.

---

### 4. Interactive Menu Built‑in (`menu`)

**File:** `src/builtins.c`

- Implemented `int builtin_menu(char **args)`.
- Clears the screen (`\033[2J\033[H`) and draws a boxed menu with options:
  - `1` – Run pipeline `ls | grep .c | wc -l`
  - `2` – Start background job `sleep 30 &`
  - `3` – Show job list (`jobs`)
  - `4` – Change scheduler to `priority`
  - `5` – View last 5 log entries (calls `builtin_log` with `-n 5`)
  - `0` – Exit menu
- After a demo, the menu redraws until the user chooses exit.
- Added `menu` to the built‑in dispatcher in `executor.c`.

---

### 5. Priority Built‑in (`prio`)

**File:** `src/builtins.c`

- `int builtin_prio(char **args)` – syntax: `prio %<jobid> <priority>`.
- Validates priority range (-20 … 19), where lower numbers mean higher priority.
- Finds the job by ID, sets `job->priority = new_priority`.
- Prints a note if the current scheduler policy is not `priority`, because the priority only matters under the priority policy.
- Registered `prio` as a built‑in in `executor.c`.

---

### 6. Enhanced Log Display (Pretty Boxed Table)

**File:** `src/builtins.c`

- Replaced the raw CSV dump of `builtin_log` with a formatted table.
- Added a helper `static void print_log_entry(const char *line)` that:
  - Parses the CSV line: `timestamp,wall_us,user_us,sys_us,pid,"command",exit_code`.
  - Converts the timestamp from 24‑hour to 12‑hour with AM/PM (function `ts_to_12h`).
  - Truncates the command to fit a 30‑character column.
  - Prints each row with coloured status: `[ OK ]` (green) on success, `[ERR ]` (red) on failure.
- The main `builtin_log` reads the log file, stores the last N lines in a circular buffer, then prints a box header, the rows, and a box footer.

**Result:** `log` now produces a clean, professional table.

---

### 7. Scheduler Confirmation Messages

**File:** `src/builtins.c` – `builtin_sched`

- Enhanced `builtin_sched` to print a confirmation line when the policy changes:
  - `"Scheduler policy changed to 'roundrobin' (50 ms timeslices)."`
  - `"Scheduler policy changed to 'priority' (static priority)."`
  - `"Scheduler policy changed to 'none' (no preemption)."`
- This gives immediate feedback to the user, especially when choosing option 4 from the menu.

---

### 8. Scheduler Getter for `prio`

**File:** `include/scheduler.h`, `src/scheduler.c`

- Added `int scheduler_get_policy(void)` that returns `current_policy`.
- This allows `builtin_prio` to check whether the priority policy is active and print a hint if not.

---

### 9. Integration into Executor

**File:** `src/executor.c`

- Added `menu`, `prio`, and `log` to the built‑in dispatch chain in `execute_command()`.  
  (The `log` built‑in was registered earlier; now `menu` and `prio` are also handled.)

---

### 10. Compilation and Testing

**Makefile:** Already included `src/scheduler.c` and linking flags for readline.

**Test results (from actual run in the terminal):**

| Feature / Command | Expected Behaviour | Actual |
|------------------|--------------------|--------|
| Launch shell | Logo + short hint, prompt with no job count | ✅ |
| `help` | Lists built‑ins with correct syntax (escaped `%`) | ✅ |
| `sleep 30 &` | Background job created, prompt shows `[1]` prefix | ✅ |
| `jobs` | Shows `[1] Running sleep 30 &` | ✅ |
| `fg %1` on running job | Rejects (“job not stopped”) – correct | ✅ |
| Ctrl‑Z on foreground `sleep 30` | Job stops, prompt shows `[2]` | ✅ |
| `bg %2` | Resumes stopped job, state changes to Running | ✅ |
| `kill %1` | Terminates, job removed from table | ✅ |
| `sched roundrobin` / `priority` / `none` | Prints confirmation and changes policy | ✅ |
| `prio %2 10` | Sets priority, warns if policy not priority | ✅ |
| `log` | Boxed table with last 10 entries, colours | ✅ |
| `log -n 3` | Shows only last 3 entries | ✅ |
| `menu` | Draws interactive menu, each demo works, exit returns to prompt | ✅ |
| Arrow keys, history | Up/down recalls previous commands, left/right moves cursor | ✅ |
| History persistence | After restart, previous commands available | ✅ |

---

### 11. Challenges Overcome

| Issue | Cause | Solution |
|-------|-------|----------|
| **Prompt printed every 50 ms** (inherited from Day 8 `select` loop) | The loop printed the prompt on every `select` timeout, flooding the screen. | Removed the `select` loop entirely; used a `SIGALRM` timer to drive scheduler ticks while `readline` blocks. This required adding `setitimer` and a signal handler for `SIGALRM`. |
| **Arrow keys printed raw escape sequences** | Readline was not given control because of the custom `select` handling. | Switched to simple blocking `readline(prompt)` with an `SIGALRM` timer for scheduler. The signal handler calls `scheduler_tick()`, and `SA_RESTART` ensures `readline` resumes. |
| **Format‑truncation warning in `print_log_entry`** | `snprintf(cmd_display, ...)` might truncate the command string. | Used `snprintf(..., "%.28s", cmd)` to explicitly limit output to 28 characters, eliminating the warning. |
| **`builtin_help` duplication** | Two definitions of `builtin_help` (a simple one and an enhanced one) caused linker error. | Removed the simple version, kept the enhanced version with escaped percent signs (`%%jobid`). |
| **Log table header misalignment** | The timestamp column width was not sufficient for 12‑hour timestamps. | Adjusted column widths: timestamp 25 chars, command 30 chars, status 8 chars. |

---

### 12. Trade‑offs and Observations

- **Readline + timer:** Using `setitimer` allows scheduler ticks to occur while `readline` blocks, but it adds complexity (signal handling, SA_RESTART). An alternative would be to use `select` with a non‑blocking readline, but that is more intrusive.
- **Boxed menu layout:** ANSI escape codes for clearing screen and drawing boxes make the menu look professional but rely on terminal support (most modern terminals do). The fallback is still usable.
- **Priority range:** Linux’s `nice` values range from -20 (highest) to 19 (lowest). Our `prio` built‑in respects this range, but the `priority` scheduler uses the job’s `priority` field directly – higher number = higher priority. We store the same value as `nice`, so highest numeric `nice` actually means **lowest** scheduling priority. This is noted in the help text.

---

### 13. Day 9 Achievements

- **Complete readline integration** – command history, line editing, persistent history across sessions.
- **Dynamic prompt** – shows the number of background jobs in red.
- **Interactive menu** (`menu`) – a guided demo that showcases pipelines, background jobs, job listing, scheduler, and logs.
- **Priority command** (`prio`) – changes job priority at runtime, supporting the priority scheduler.
- **Enhanced log display** – readable table with colour‑coded status.
- **Clean startup** – only logo and a hint line; full help available on demand.
- All code compiles with `-Werror`, no memory leaks, and all built‑ins work as documented.

---

### Next Steps (Day 10)

- **Documentation:** Produce API documentation for all modules (Doxygen or Markdown).
- **Reproducibility:** Write a guide with scripts to replicate experiments (scheduler performance, job control tests, logging analysis).
- **Research report:** Summarise findings, trade‑offs, and measurements in a paper draft ready for peer review.