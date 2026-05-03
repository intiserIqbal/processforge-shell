#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "jobs.h"

enum sched_policy
{
    SCHED_NONE,
    SCHED_RR,
    SCHED_PRIO
};

void scheduler_init(void);
void scheduler_set_policy(enum sched_policy pol);
const char *sched_get_policy_name(void);
int scheduler_get_policy(void); // NEW: returns current policy
void scheduler_add_job(job_t *j);
void scheduler_remove_job(pid_t pgid);
void scheduler_tick(void);

#endif