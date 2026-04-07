# ProcessForge Shell — Development Log

## Day 4: Multi‑Stage Pipelines, Prompt Refinement & Test Suite Expansion

### Objective

The goal for Day 4 was to extend pipeline support beyond single‑stage, refine the shell prompt for both interactive and scripted use, and expand the test suite to validate new functionality.

---

## 1. Multi‑Stage Pipeline Support

**Files:**  
`parser.c`, `executor.c`, `shell.h`

- Introduced `MAX_COMMANDS` constant to distinguish between arguments per command and number of commands in a pipeline.  
- Updated `Pipeline` struct to store multiple commands cleanly.  
- Refactored `parse_input()` to handle any number of commands separated by `|`.  
- Rewrote `execute_pipeline()` to dynamically connect commands using `pipe()` and `dup2()`.  
- Removed duplicate pipeline executor definitions for consistency.  
- Pipelines like `ls | grep .c | sort | wc -l` now execute correctly.

---

## 2. Prompt Refinement

**Files:**  
`main.c`

- Added `isatty(STDIN_FILENO)` check to suppress prompts when shell is run with scripted input.  
- Interactive sessions still show `processforge>` prompt.  
- Scripted tests now produce clean output without repeated prompts.  
- Explored dynamic prompt options (e.g., `processforge:/cwd>`), laying groundwork for future usability improvements.

---

## 3. Test Suite Expansion

**Files:**  
`tests/test_builtins.sh`, `tests/test_pipes.sh`, `tests/test_redirection.sh`

- Added multi‑stage pipeline tests (`ls | grep .c | sort | wc -l`).  
- Added redirection append and input redirection cases (`echo world >> test.txt`, `cat < test.txt`).  
- Added `cd` and `pwd` builtin tests.  
- Cleaned up redundant test artifacts (`tests.c`, `tests/temp/`), keeping only fixtures and expected outputs.  
- Tests now cover pipelines, redirection, and builtins comprehensively.

---

## 4. Observed Issues

- Background jobs (`&`) execute but are not tracked in a job table yet.  
- No job control commands (`jobs`, `fg`, `bg`) implemented.  
- Prompt currently shows only `processforge>`; dynamic contextual prompt (`processforge:/cwd>`) is planned but not yet integrated.  
- Instrumentation for performance metrics (latency, turnaround time) still missing.

---

## Day 4 Achievements

- Full multi‑stage pipeline support.  
- Cleaner prompt handling for interactive vs. scripted use.  
- Expanded test coverage for pipelines, redirection, and builtins.  
- Repository hygiene improved by removing redundant test files.  
- Foundation laid for job control and scheduler experiments.

---

**Next Steps:**  
- Implement job control (`jobs`, `fg`, `bg`) with a job table.  
- Add instrumentation for process lifecycle logging.  
- Begin scheduler module (FCFS, SJF, RR).  
- Enhance prompt with contextual information (current directory, job count).  
- Continue expanding automated test coverage.

---