#define _POSIX_C_SOURCE 200809L
#include "../include/logging.h"
#include "../include/jobs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include <time.h>
#include <errno.h>

extern job_t jobs[MAX_JOBS];
extern int job_count;

static FILE *log_file = NULL;

#define DEFERRED_MAX 64
static int deferred_pgids[DEFERRED_MAX];
static int deferred_head = 0;
static int deferred_tail = 0;
static int deferred_count = 0;

int init_logging(const char *log_path)
{
    (void)log_path; /* ignore user path, use fixed /tmp location */
    const char *path = "/tmp/processforge.log";
    log_file = fopen(path, "a");
    if (!log_file)
    {
        fprintf(stderr, "init_logging: failed to open %s: %s\n", path, strerror(errno));
        return -1;
    }
    setvbuf(log_file, NULL, _IOLBF, 0);
    //fprintf(stderr, "Logging enabled: %s\n", path);
    return 0;
}

void log_job_finish(int pgid, const char *cmd, int exit_status,
                    const struct rusage *ru, const struct timespec *start)
{
    if (!log_file)
        return;
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    long wall_ms = (end.tv_sec - start->tv_sec) * 1000 +
                   (end.tv_nsec - start->tv_nsec) / 1000000;
    long user_ms = ru->ru_utime.tv_sec * 1000 + ru->ru_utime.tv_usec / 1000;
    long sys_ms = ru->ru_stime.tv_sec * 1000 + ru->ru_stime.tv_usec / 1000;

    time_t now = time(NULL);
    struct tm tm_buf;
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime_r(&now, &tm_buf));

    fprintf(log_file, "%s,%ld,%ld,%ld,%d,\"%s\",%d\n",
            time_str, wall_ms, user_ms, sys_ms, pgid, cmd, exit_status);
    fflush(log_file);
}

void enqueue_deferred_log(int pgid)
{
    if (deferred_count >= DEFERRED_MAX)
        return;
    deferred_pgids[deferred_tail] = pgid;
    deferred_tail = (deferred_tail + 1) % DEFERRED_MAX;
    deferred_count++;
}

void process_deferred_logs(void)
{
    while (deferred_count > 0)
    {
        int pgid = deferred_pgids[deferred_head];
        deferred_head = (deferred_head + 1) % DEFERRED_MAX;
        deferred_count--;

        job_t *job = NULL;
        for (int i = 0; i < job_count; i++)
        {
            if (jobs[i].pgid == pgid && !jobs[i].logged)
            {
                job = &jobs[i];
                break;
            }
        }
        if (!job)
            continue;

        struct rusage ru;
        getrusage(RUSAGE_CHILDREN, &ru);
        log_job_finish(job->pgid, job->command, job->exit_status,
                       &ru, &job->start_time);
        job->logged = 1;
    }
}

void close_logging(void)
{
    if (log_file)
        fclose(log_file);
}

/* Stub for log_job_start – not needed */
void log_job_start(int pgid, const char *cmd)
{
    (void)pgid;
    (void)cmd;
}