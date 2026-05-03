#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "../include/scheduler.h"
#include "../include/jobs.h"

static job_t **bg_jobs = NULL;
static int bg_jobs_cap = 0;
static int bg_jobs_cnt = 0;

static enum sched_policy current_policy = SCHED_NONE;
static int rr_current_idx = -1;

static void stop_job(job_t *j)
{
    if (!j)
        return;
    if (kill(-j->pgid, SIGSTOP) < 0 && errno != ESRCH)
        perror("stop_job: kill(SIGSTOP) failed");
}

static void continue_job(job_t *j)
{
    if (!j)
        return;
    if (kill(-j->pgid, SIGCONT) < 0 && errno != ESRCH)
        perror("continue_job: kill(SIGCONT) failed");
}

static void rebalance_scheduler(void)
{
    if (bg_jobs_cnt == 0)
        return;

    switch (current_policy)
    {
    case SCHED_NONE:
        for (int i = 0; i < bg_jobs_cnt; i++)
            continue_job(bg_jobs[i]);
        break;

    case SCHED_RR:
        if (rr_current_idx < 0 || rr_current_idx >= bg_jobs_cnt)
            rr_current_idx = 0;
        for (int i = 0; i < bg_jobs_cnt; i++)
        {
            if (i == rr_current_idx)
                continue_job(bg_jobs[i]);
            else
                stop_job(bg_jobs[i]);
        }
        break;

    case SCHED_PRIO:
    {
        int best_idx = 0;
        for (int i = 1; i < bg_jobs_cnt; i++)
        {
            if (bg_jobs[i]->priority > bg_jobs[best_idx]->priority)
                best_idx = i;
        }
        for (int i = 0; i < bg_jobs_cnt; i++)
        {
            if (i == best_idx)
                continue_job(bg_jobs[i]);
            else
                stop_job(bg_jobs[i]);
        }
        break;
    }
    }
}

void scheduler_init(void)
{
    current_policy = SCHED_NONE;
    rr_current_idx = -1;
}

void scheduler_set_policy(enum sched_policy pol)
{
    if (current_policy == pol)
        return;
    current_policy = pol;
    rebalance_scheduler();
}

/* NEW: Return current policy */
int scheduler_get_policy(void)
{
    return current_policy;
}

const char *sched_get_policy_name(void)
{
    switch (current_policy)
    {
    case SCHED_NONE:
        return "none";
    case SCHED_RR:
        return "roundrobin";
    case SCHED_PRIO:
        return "priority";
    default:
        return "unknown";
    }
}

void scheduler_add_job(job_t *j)
{
    if (bg_jobs_cnt >= bg_jobs_cap)
    {
        bg_jobs_cap = bg_jobs_cap ? bg_jobs_cap * 2 : 4;
        job_t **new_arr = realloc(bg_jobs, sizeof(job_t *) * bg_jobs_cap);
        if (!new_arr)
        {
            perror("scheduler_add_job realloc");
            return;
        }
        bg_jobs = new_arr;
    }
    bg_jobs[bg_jobs_cnt++] = j;
    if (j->priority == 0)
        j->priority = 0;
    rebalance_scheduler();
}

void scheduler_remove_job(pid_t pgid)
{
    for (int i = 0; i < bg_jobs_cnt; i++)
    {
        if (bg_jobs[i]->pgid == pgid)
        {
            for (int j = i; j < bg_jobs_cnt - 1; j++)
                bg_jobs[j] = bg_jobs[j + 1];
            bg_jobs_cnt--;
            if (rr_current_idx >= bg_jobs_cnt)
                rr_current_idx = -1;
            else if (rr_current_idx == i)
                rr_current_idx = -1;
            rebalance_scheduler();
            return;
        }
    }
}

void scheduler_tick(void)
{
    if (current_policy != SCHED_RR)
        return;
    if (bg_jobs_cnt == 0)
        return;

    if (rr_current_idx >= 0 && rr_current_idx < bg_jobs_cnt)
        stop_job(bg_jobs[rr_current_idx]);
    rr_current_idx = (rr_current_idx + 1) % bg_jobs_cnt;
    continue_job(bg_jobs[rr_current_idx]);
}