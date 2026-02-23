#include "job.h"
#include "csapp.h"

job_t jobs[MAXJOBS];
static int nextjid = 1;

int find_free_cell(){
    for(int i=0; i<MAXJOBS; i++){
        if(jobs[i].state == UNDEF){
            return i;
        }
    }
    return -1; // tableau est plein
}

int fg_job_jid() {
    for (int i = 0; i < MAXJOBS; i++) {
        if (jobs[i].state == FG)
            return jobs[i].jid;
    }
    return 0;
}

job_t* get_job_by_jid(int jid){
    for (int i = 0; i < MAXJOBS; i++) {
        if (jobs[i].state != UNDEF && jobs[i].jid == jid)
            return &jobs[i]; 
    }
    return NULL;
}

job_t* get_job_by_pgid(int pgid){
    for (int i = 0; i < MAXJOBS; i++) {
        if (jobs[i].state != UNDEF && jobs[i].pgid == pgid)
            return &jobs[i]; 
    }
    return NULL;
}

job_t* get_job_by_pid(int pid){
    for (int i = 0; i < MAXJOBS; i++) {
        for(int j=0; j<jobs[i].nbproc; j++){
            if (jobs[i].state != UNDEF && jobs[i].pids[j] == pid)
                return &jobs[i]; 
        }
    }
    return NULL;
}

int add_job(pid_t pgid, int* pids, int nbproc, job_state state, char *cmdline){
    if (pgid<=0) return -1;

    int i = find_free_cell();
    if (i<0) return -1;  // tableau est plein

    jobs[i].pgid = pgid;
    jobs[i].nbproc = nbproc;
    jobs[i].nbprocactiv = nbproc;
    for(int k=0; k<nbproc; k++){
        jobs[i].pids[k] = pids[k];
    }
    jobs[i].state = state;
    jobs[i].jid = nextjid++;
    strncpy(jobs[i].cmdline, cmdline, sizeof(jobs[i].cmdline) - 1);
    jobs[i].cmdline[sizeof(jobs[i].cmdline) - 1] = '\0';

    return jobs[i].jid;
}

int delete_job(int jid){
    for(int i=0; i<MAXJOBS; i++){
        if(jobs[i].jid == jid){
            jobs[i].jid = 0;
            jobs[i].pgid = 0;
            for(int j=0; j<MAXNBPROC; j++){
                jobs[i].pids[j] = 0;
            }
            jobs[i].nbproc = 0;
            jobs[i].nbprocactiv = 0;
            jobs[i].state = UNDEF;
            jobs[i].cmdline[0] = '\0';
            return 0;
        }
    }
    return -1;
}

void init_jobs(){
    for(int i=0; i<MAXJOBS; i++){
        jobs[i].jid = 0;
        jobs[i].pgid = 0;
        for(int j=0; j<MAXNBPROC; j++){
            jobs[i].pids[j] = 0;
        }
        jobs[i].nbproc = 0;
        jobs[i].nbprocactiv = 0;
        jobs[i].state = UNDEF;
        jobs[i].cmdline[0] = '\0';
    }
}
