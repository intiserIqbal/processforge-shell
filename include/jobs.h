#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>

#define MAX_JOBS 64
#define MAX_CMD_LEN 256

typedef enum
{
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_DONE
} job_state_t;

typedef struct
{
    int id;
    pid_t pgid;
    char command[MAX_CMD_LEN];
    job_state_t state;

    int total_processes;  // NEW
    int active_processes; // NEW
} job_t;

void init_jobs();
int add_job(pid_t pgid, const char *cmd, job_state_t state);
void remove_job(pid_t pgid);
void update_job_state(pid_t pgid, job_state_t state);
void print_jobs();
job_t *find_job_by_id(int id);
job_t *find_job_by_pgid(pid_t pgid);

#endif