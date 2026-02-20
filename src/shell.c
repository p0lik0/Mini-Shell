/*
 * Copyright (C) 2002, Simon Nieuviarts
 */

#include <stdio.h>
#include <stdlib.h>
#include "readcmd.h"
#include "csapp.h"

#define MAX67 1024

int main()
{
	while (1) {
		struct cmdline *l;
		int i, j;

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
		// tableau de discripteurs pour des pipes
		for (i=0; l->seq[i]!=0; i++) { 
			seq_l++;
		}
		for (i=0; l->seq[i]!=0; i++) { // pour chaque commande d une suite de commandes

			if(strcmp(l->seq[i][0], "quit")==0){
				exit(0);
			}

			int pid;
			
			if ((pid = Fork()) == 0) {
				// si il y a une commande après la commande courrante
				// (donc si on doit communiquer l output à un processus suivant)
				if(l->seq[i+1]!=0){ 
					int fd[2];
					char buf[MAX67];
					pipe(fd);
				}
				if (l->in) {
					int fd_in = open(l->in, O_RDONLY);
					if (fd_in < 0) {
						perror(l->in);
						exit(1);
					}
					dup2(fd_in, 0);
					close(fd_in);
				}

				if (l->out) {
					int fd_out = open(l->out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
					if (fd_out < 0) {
						perror(l->out);
						exit(1);
					}
					dup2(fd_out, 1);
					close(fd_out);
				}

				execvp(l->seq[0][0], &(l->seq[0][0]));
				
				if (errno == ENOENT) {
					fprintf(stderr, "%s: command not found\n", l->seq[0][0]);
				} else {
					perror(l->seq[0][0]);
				}
				exit(1);
			}

			waitpid(pid, NULL, 0);
		}
		/* Display each command of the pipe */
		// for (i=0; l->seq[i]!=0; i++) {
		// 	char **cmd = l->seq[i];
		// 	if(strcmp(cmd[0], "quit")==0){
		// 		exit(0);
		// 	}
		// 	printf("seq[%d]: ", i);
		// 	for (j=0; cmd[j]!=0; j++) {
		// 		printf("%s ", cmd[j]);
		// 	}
		// 	printf("\n");
		// }
	}
}
