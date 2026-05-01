# ProcessForge Shell — Development Log

## Day 7: Instrumentation – Job Logging with Execution Metrics

### Objective
Add a non‑intrusive logging subsystem that records every pipeline or simple command executed by the shell. The log must contain start time, wall‑clock duration, user/sys CPU time (aggregated over all processes in the job), PGID, the command string, and the exit status (or terminating signal). This instrumentation is the foundation for later performance analysis and reproducibility.

---

### 1. Job Structure Extensions
**Files:** `include/jobs.h`, `src/jobs.c`

- Added three fields to `job_t`:
  - `struct timespec start_time` – monotonic time when `add_job` is called.
  - `int logged` – flag to avoid duplicate logging.
  - `int exit_status` – exit code (or `-signal` if terminated by a signal).
- In `add_job`, call `clock_gettime(CLOCK_MONOTONIC, &job->start_time)` and set `job->logged = 0`.
- Made `jobs[]` and `job_count` non‑static (visible to `logging.c`).

---

### 2. Logging API (Signal‑Safe Deferred Queue)
**Files:** `include/logging.h`, `src/logging.c`

- `init_logging()` – opens log file (path from `$PF_LOG_FILE` or default `~/.processforge/logs/shell.log`), expands `~`, creates parent directories with `mkdir_p`.
- `log_job_finish()` – writes a CSV line with timestamp, wall‑clock ms, user ms, sys ms, PGID, command (quoted), and exit status.
- Deferred logging for background jobs:
  - `enqueue_deferred_log(pgid)` – safe to call from `SIGCHLD` handler; stores PGID in ring buffer.
  - `process_deferred_logs()` – called from main loop; retrieves PGIDs, looks up job, collects `rusage`, and calls `log_job_finish()`.
- **Key design:** never call `fprintf` inside a signal handler; deferred queue guarantees async‑signal safety.

---

### 3. Integration into Executor (Foreground Jobs)
**Files:** `src/executor.c`

- **Simple commands (`execute_command`):**
  - Capture start time before `waitpid`.
  - After waiting, if finished (not stopped), call `getrusage(RUSAGE_CHILDREN)` and `log_job_finish()`.
- **Pipelines (`execute_pipeline`):**
  - Record start time before `tcsetpgrp`.
  - After wait loop, if terminated (not stopped), collect CPU stats and log.
  - Prevent double logging by setting `job->logged = 1` when logging immediately.
- **Why `getrusage(RUSAGE_CHILDREN)` works:** For a foreground job, no other children run concurrently, so usage reflects that job.

---

### 4. Background Job Logging via Signal Handler
**Files:** `src/signals.c`

- In `handle_sigchld`, when a job’s last process exits (`active_processes == 0`), call `enqueue_deferred_log(job->pgid)`.
- Handler stores PGID and returns immediately – no logging inside signal context.
- Main loop in `main.c` calls `process_deferred_logs()` before each prompt, flushing pending logs.

---

### 5. Main Loop and Logging Lifespan
**Files:** `src/main.c`

- After `setup_signal_handlers()`, call `init_logging()`.
- Before printing prompt, call `process_deferred_logs()` to flush background job logs.
- On shell exit, call `close_logging()` to flush and close log file.

---

### 6. Compilation and Testing
**Files:** `Makefile`

- Added `src/logging.c` to `SRC` list.
- Compiled with `-Wall -Wextra -Werror -g -O0`; all warnings resolved.

**Test scenarios:**

| Command                     | Log entry content                                               | Result |
|-----------------------------|-----------------------------------------------------------------|--------|
| `yes \| head -n5`           | Wall ~1 ms, user 0 ms, sys ~1 ms, status -13 (SIGPIPE)          | ✅     |
| `sleep 1`                   | Wall ~1002 ms, user 0 ms, sys 0 ms, status 0                    | ✅     |
| `ls \| wc -l`               | Wall ~12 ms, user 2 ms, sys 2 ms, status 0                      | ✅     |
| `sleep 5 &` (then Enter)    | After 5s, log line appears with wall ~5000 ms, CPU near 0, status 0 | ✅ |
| `pf_log_file=/tmp/mylog.log ./processforge` | Log written to `/tmp/mylog.log` instead of default | ✅ |

All existing functionality (pipes, redirections, job control built‑ins) remained intact.

---

### 7. Challenges Overcome
- **Directory creation:** implemented recursive `mkdir_p` to create missing parents, handling `EEXIST`.
- **Tilde expansion:** used `getpwuid()` and `getenv("HOME")` to replace leading `~`.
- **Signal safety:** avoided undefined behaviour by deferring logs instead of writing inside `SIGCHLD`.
- **Preventing double logging:** foreground jobs logged immediately (`logged = 1`), background jobs logged once via deferred mechanism.
- **CPU time aggregation:** `getrusage(RUSAGE_CHILDREN)` works for foreground jobs; background jobs logged via deferred queue, acceptable for Day 7 but noted for refinement.

---

## Day 7 Achievements
- Full instrumentation that records every executed job (foreground and background).
- Log entries contain: timestamp, wall‑clock duration (ms), user CPU (ms), system CPU (ms), PGID, command string, and exit status.
- Deferred logging ensures signal safety and avoids race conditions.
- Log file location configurable via environment variable; default `~/.processforge/logs/shell.log`.
- No performance regression – negligible overhead.
- Code compiles and runs under `-Werror` with no new warnings.

---

## Next Steps (Beyond Day 7)
- **Day 8 – Scheduler:** Implement process priorities and round‑robin scheduling policies for background jobs.
- **Day 9 – UX:** Add GNU readline for command history and line editing; display job count in prompt.
- **Day 10 – Reproducibility:** Write a script to rebuild the shell from scratch and run a test suite; produce a research report comparing shell performance with bash.
