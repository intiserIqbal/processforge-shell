#include <stdio.h>
#include <string.h>
#include "../include/jobs.h"

static job_t jobs[MAX_JOBS];
static int job_count = 0;
static int next_job_id = 1;

void init_jobs()
{
    memset(jobs, 0, sizeof(jobs));
    job_count = 0;
    next_job_id = 1;
}

int add_job(pid_t pgid, const char *cmd, job_state_t state)
{
    if (job_count >= MAX_JOBS)
        return -1;

    job_t *job = &jobs[job_count++];

    job->id = next_job_id++;
    job->pgid = pgid;
    job->state = state;

    job->total_processes = 1;
    job->active_processes = 1;

    strncpy(job->command, cmd, MAX_CMD_LEN - 1);
    job->command[MAX_CMD_LEN - 1] = '\0';

    return job->id;
}

void remove_job(pid_t pgid)
{
    for (int i = 0; i < job_count; i++)
    {
        if (jobs[i].pgid == pgid)
        {
            jobs[i] = jobs[job_count - 1];
            job_count--;
            return;
        }
    }
}

void update_job_state(pid_t pgid, job_state_t state)
{
    for (int i = 0; i < job_count; i++)
    {
        if (jobs[i].pgid == pgid)
        {
            jobs[i].state = state;
            return;
        }
    }
}

job_t *find_job_by_id(int id)
{
    for (int i = 0; i < job_count; i++)
    {
        if (jobs[i].id == id)
            return &jobs[i];
    }
    return NULL;
}

job_t *find_job_by_pgid(pid_t pgid)
{
    for (int i = 0; i < job_count; i++)
    {
        if (jobs[i].pgid == pgid)
            return &jobs[i];
    }
    return NULL;
}

void print_jobs()
{
    for (int i = 0; i < job_count; i++)
    {
        if (jobs[i].state == JOB_DONE)
        {
            printf("[%d] Done %s\n", jobs[i].id, jobs[i].command);
            remove_job(jobs[i].pgid);
            i--;
            continue;
        }

        const char *state_str =
            jobs[i].state == JOB_RUNNING ? "Running" : jobs[i].state == JOB_STOPPED ? "Stopped"
                                                                                    : "Done";

        printf("[%d] %s %s\n", jobs[i].id, state_str, jobs[i].command);
    }
}