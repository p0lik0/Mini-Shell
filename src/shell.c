/*
 * Copyright (C) 2002, Simon Nieuviarts
 */

#include <stdio.h>
#include <stdlib.h>
#include "readcmd.h"
#include "csapp.h"
#include <signal.h>

#define MAX67 1024

void sigchld_handler(int sig) {
	int saved_errno = errno;
	int pid;
	// tq on a des fils en etat de zombie on les ramasse
	while ((pid = waitpid(-1, NULL,  WNOHANG|WUNTRACED)) > 0);

	// si on a une erreur outre celle que tous les fils ont été déjà ramassés 
	if(pid==-1 && errno!=ECHILD) unix_error("waitpid error");

	errno = saved_errno;
}


int main(){
	Signal(SIGCHLD, sigchld_handler);

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
		int pids[seq_l]; // tableau pour memoriser des pids, pour une bonne gestion de waitpid
		for (int i=0; i<seq_l; i++) { // pour chaque commande d une suite de commandes
			
			// creation d un pipe pour faire communiquer fils courrant et fils executant la commande suivante
			if (i<seq_l-1) {
				if (pipe(pipes[i]) < 0) {
					perror("pipe");
					exit(1);
				}
			}

			int pid;
			if ((pid = Fork()) == 0) { //si fils

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
			pids[i] = pid; // on sauvegarde le pid du fils crée
		}
		if(!l->is_on_backgr){
			for(int i=0; i<seq_l; i++){
				while(waitpid(pids[i], NULL, 0) < 0){ 
					// dans le cas où waitpid a été interrompu par sigchld_handler - continuer la boucle 
					if(errno == EINTR) continue; 
					// dans le cas où le fils pids[i] a été déjà ramassé par handler - break la boucle 
					else if(errno==ECHILD) break; 
					else{ // sinon c une comportement non prevue 
						perror("waitpid"); 
						break;
					}
				}
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
