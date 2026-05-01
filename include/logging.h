#ifndef LOGGING_H
#define LOGGING_H

#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>

/* Initialize logging module.
 * log_path: absolute or relative path to log file (directories created).
 * Returns 0 on success, -1 on failure (logs to stderr). */
int init_logging(const char *log_path);

/* Record that a job has started.
 * pgid: process group ID, cmd: full command string. */
void log_job_start(int pgid, const char *cmd);

/* Record that a job has finished.
 * pgid: process group ID, cmd: full command string.
 * exit_status: exit code (or -signal if terminated by signal).
 * ru: rusage as returned by wait4/getrusage.
 * start: monotonic start time (from clock_gettime). */
void log_job_finish(int pgid, const char *cmd, int exit_status,
                    const struct rusage *ru, const struct timespec *start);

/* Called from SIGCHLD handler (async‑signal‑safe) to defer logging.
 * Stores only the PGID; main loop will process later. */
void enqueue_deferred_log(int pgid);

/* Process any deferred log entries (e.g., from SIGCHLD handler).
 * Must be called from main loop, NOT from signal handler. */
void process_deferred_logs(void);

/* Close log file, flush pending writes. */
void close_logging(void);

#endif