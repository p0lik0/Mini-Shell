/*
 * Copyright (C) 2002, Simon Nieuviarts
 */

#include <stdio.h>
#include <stdlib.h>
#include "readcmd.h"
#include "csapp.h"
#include <signal.h>
#include "job.h"

#define MAX67 1024

void sigchld_handler(int sig) {
    int olderrno = errno;
    int status;
    int pid;
	// tq on a des fils en etat de zombie on les ramasse
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {

        job_t *j = get_job_by_pid(pid);
        if (!j)
            continue;

        if (WIFSTOPPED(status)) {
            j->state = ST;
        }
        else if (WIFSIGNALED(status) || WIFEXITED(status)) {
			j->nbprocactiv--;
			if(j->nbprocactiv==0)
            	delete_job(j->jid);
        }
    }

	// si on a une erreur outre celle que tous les fils ont été déjà ramassés 
	if(pid==-1 && errno!=ECHILD) unix_error("waitpid error");


    errno = olderrno;
}

void sigtstp_handler(){
	int fg_jid = fg_job_jid();
	if (fg_jid != 0) {
		job_t *j = get_job_by_jid(fg_jid);
		Kill(-j->pgid, SIGTSTP);
	}
}

void sigint_handler(){
    int fg_jid = fg_job_jid();
    if (fg_jid != 0) {
        job_t *j = get_job_by_jid(fg_jid);
        Kill(-j->pgid, SIGINT);
    }
}

char *restore_cmdline(char ***seq){
	char *cmdline = malloc(1024);
	cmdline[0] = '\0';

	for(int i = 0; seq[i] != NULL; i++){
		for(int j = 0; seq[i][j] != NULL; j++){
			strcat(cmdline, seq[i][j]);
			strcat(cmdline, " ");
		}
		if (seq[i+1] != NULL)
			strcat(cmdline, "| ");
	}

	return cmdline;
}

int main(){
	Signal(SIGCHLD, sigchld_handler);
	Signal(SIGINT, sigint_handler);
	Signal(SIGTSTP, sigtstp_handler);

	sigset_t mask_one, prev_one;
	Sigemptyset(&mask_one);
	Sigaddset(&mask_one, SIGCHLD);


	while (1) {
		struct cmdline *l;

		printf("shell> ");
		l = readcmd();

		/* If input stream closed, normal termination */
		if (!l) {
			printf("exit\n");
			exit(0);
		}

		if (l->err) {
			/* Syntax error, read another command */
			printf("error: %s\n", l->err);
			continue;
		}
		int seq_l = 0;
		// calculons nb de commandes passées
		for (int i=0; l->seq[i]!=0; i++) { 
			seq_l++;
		}
		if (seq_l == 0) continue; // si 0 commandes continue boucle

		if(strcmp(l->seq[0][0], "quit")==0){ // si la commande courrante est "quit" == commence par "quit"
			exit(0);
		}


		int pipes[seq_l-1][2] ; // tableau de descripteurs pour des pipes

		int pgid = 0;
		int pids[MAXNBPROC];
		for (int i=0; i<seq_l; i++) { // pour chaque commande d une suite de commandes
			
			// creation d un pipe pour faire communiquer fils courrant et fils executant la commande suivante
			if (i<seq_l-1) {
				if (pipe(pipes[i]) < 0) {
					perror("pipe");
					exit(1);
				}
			}

			// masker SIGCHLD pour eviter la situation 
			// où le fils meurt avant d'être mis dans le tableau de jobs
			Sigprocmask(SIG_BLOCK, &mask_one, &prev_one);
			int pid;
			if ((pid = Fork()) == 0) { //si fils
				// unblock SIGCHLD
				Sigprocmask(SIG_SETMASK, &prev_one, NULL);

				// si c la 1re commande de pipe, alors le job de ce pipe portera 
				// le pgid = son pid
				if(pgid == 0) setpgid(0,0);
				else setpgid(0,pgid);

				if (l->in && i==0) { // si on a un input non standart 
				// (dans le cas de sequence de commandes pipées possible que pour la 1re commande)
					int fd_in = open(l->in, O_RDONLY);
					if (fd_in < 0) {
						perror(l->in);
						exit(1);
					}
					dup2(fd_in, 0); // remplacement de input original
					close(fd_in);
				}
				else if (l->is_on_backgr && i==0) {
					int fd_null = open("/dev/null", O_RDONLY);
					if (fd_null<0) {
						perror("/dev/null");
						exit(1);
					}
					dup2(fd_null, 0);
					close(fd_null);
				}
				else if(i>0){ // si ce n est pas la 1re commande dans une sequence de commandes
					dup2(pipes[i-1][0], 0); 
				}

				if (l->out && i==seq_l-1) { // si on a un output non standart 
				// (dans le cas de sequence de commandes pipées possible que pour la derniere commande)
					int fd_out = open(l->out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
					if (fd_out < 0) {
						perror(l->out);
						exit(1);
					}
					dup2(fd_out, 1);
					close(fd_out);
				}
				// si il y a une commande après la commande courrante
				// (donc si on doit communiquer l output à un processus suivant)
				else if(i<seq_l-1){ 
					dup2(pipes[i][1],1);
				}

				// fermeture de pipes pour des commandes precendentes
				for(int k=0; k<i; k++){
					close(pipes[k][0]);
					close(pipes[k][1]);
				}
				// fermeture de pipe courrante
				//(sauf dans le cas de derniere commande, car il n y a pas de pipe pour elle)
				if(i<seq_l-1){
					close(pipes[i][0]);
					close(pipes[i][1]);
				}

				execvp(l->seq[i][0], &(l->seq[i][0]));
				// gestion des erreur de exec
				if (errno == ENOENT) {
					fprintf(stderr, "%s: command not found\n", l->seq[i][0]);
				} else {
					perror(l->seq[i][0]);
				}
				exit(1);
			}

			if(i>0){
				close(pipes[i-1][0]);
				close(pipes[i-1][1]);
			}
			// si le pgid n etait pas definit, alors c le fils qu on vient de cree est le 1re
			// et son pid sera le pgid de ce job
			if(pgid==0) pgid = pid;

			setpgid(pid, pgid);
			pids[i] = pid;
		}
		char * cmdline =  restore_cmdline(l->seq);

		add_job(pgid, pids, seq_l, l->is_on_backgr ? BG : FG, cmdline);
		Sigprocmask(SIG_SETMASK, &prev_one, NULL);

		free(cmdline);

		if(!l->is_on_backgr){
			// tq il existe un job en premier plan
			while(fg_job_jid()!=0){
				sleep(1);
			}
		}

		/* Display each command of the pipe */
		// for (int i=0; l->seq[i]!=0; i++) {
		// 	char **cmd = l->seq[i];
		// 	if(strcmp(cmd[0], "quit")==0){
		// 		exit(0);
		// 	}
		// 	printf("seq[%d]: ", i);
		// 	for (int j=0; cmd[j]!=0; j++) {
		// 		printf("%s ", cmd[j]);
		// 	}
		// 	printf("\n");
		// }
	}
}
