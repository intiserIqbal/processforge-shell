# ProcessForge Shell — Development Log

## Day 5: Job Control Foundations, Modular Signals & Async Reaping

### Objective

The goal for Day 5 was to lay the foundation for job control, modularize all signal handling, and implement asynchronous child reaping to prevent zombies and enable future job management features.

---

## 1. Job Control Foundations & Modular Signal Handling

**Files:**  
`src/parser.c`, `src/executor.c`, `src/main.c`, `shell.h`, `src/jobs.c`, `src/signals.c`, `include/signals.h`

- Created `signals.c` and `signals.h` to centralize all signal handling logic, decoupling it from `main.c`.
- Implemented a robust SIGCHLD handler using `sigaction()` and `waitpid(-1, ..., WNOHANG)` to reap all terminated children and update job state.
- Updated `main.c` to call `setup_signal_handlers()` and removed direct SIGCHLD logic from the main loop.
- Ensured all background jobs are reaped asynchronously, preventing zombie processes.
- Updated `jobs.c` so DONE jobs are printed once and then removed from the job table.
- All job state transitions and process reaping are now event-driven, mirroring real shell/OS behavior.

---

## 2. Prompt & Pipeline Improvements (Carried from Day 4)

**Files:**  
`src/main.c`, `src/parser.c`, `src/executor.c`, `shell.h`

- Retained prompt refinement: prompt is only shown in interactive mode (`isatty`).
- Multi-stage pipelines and redirection logic remain robust and covered by tests.

---

## 3. Test Suite Expansion & Automation

**Files:**  
`tests/test_pipes.sh`, `tests/test_redirection.sh`

- Added/updated tests for multi-stage pipelines, input/output redirection, and builtins.
- Confirmed that background jobs are reaped and removed from the job table after completion.
- Automated test runs verify prompt suppression in non-interactive/scripted mode.

---

## 4. Observed Issues

- Job table currently only supports single-process jobs (PID == PGID); pipelines will require enhanced tracking.
- SIGINT handling is still in `main.c` (to be moved to `signals.c` for full modularity).
- Memory leak in `expand_glob()` not yet addressed.
- No terminal control (`tcsetpgrp`) or stopped job handling yet.

---

## Day 5 Achievements

- Modular, event-driven signal handling (SIGCHLD) and job state management.
- No zombie processes; background jobs are tracked and cleaned up.
- Prompt and pipeline features from Day 4 remain stable and tested.
- Test suite expanded for pipelines, redirection, and job control basics.
- Foundation laid for advanced job control (jobs, fg, bg) and scheduler integration.

---

**Next Steps:**
- Design job table data structures and API in `shell.h` for multi-process jobs.
- Implement job registration on background fork and job lookup utilities.
- Add instrumentation hooks to executor for process lifecycle logging.
- Begin scheduler module (FCFS, SJF, RR).
- Move SIGINT handling to `signals.c` for full modularity.
- Continue expanding automated test coverage.

---
