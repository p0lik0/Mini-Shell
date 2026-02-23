#ifndef _JOB_H_
#define _JOB_H_
#define MAXJOBS 10
#define MAXNBPROC 10

typedef enum { UNDEF, FG, BG, ST } job_state;

typedef struct {
    int jid;
    int pgid;
    int pids[MAXNBPROC]; // tableau de pids de processuses du job
    int nbproc; // nb de processuses qui sont en etat d execution (qui ne sont pas terminés)
    int nbprocactiv;
    job_state state;    
    char cmdline[1024];
} job_t;

// donne le numéro de la première case libre du tableau
int find_free_cell(); 

// donne le numéro du job de premier plan (s’il existe)
int fg_job_jid() ;

// renvoie un pointeur vers le job
job_t* get_job_by_jid(int jid);

// renvoie un pointeur vers le job
job_t* get_job_by_pgid(int pgid);

job_t* get_job_by_pid(int pid);

// ajouter un nouveau job
// Valeur de retour : jid ou -1 dans le cas d erreur
int add_job(int pid, int pids[], int nbproc, job_state state, char *cmdline);

// supprimer un job et libérer la case correspondante du tableau
int delete_job(int jid);

// initialiser le tableau.
void init_jobs();

// afficher les jobs
void print_jobs();

#endif