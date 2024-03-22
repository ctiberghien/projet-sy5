#include <readline/readline.h>
#include <readline/history.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <poll.h>
#include <sys/select.h>
#include <fcntl.h>
#include "format_prompt.h"
#include "works_utils.h"
#include "terminal.h"
#include "args_utils.h"

int cd(char* args) {
  if(args==NULL) {
    args = getenv("HOME");
  }
  if (strcmp(args, "-")==0){
    args = getenv("OLDPWD");
    if (chdir(args) != 0) {
      printf("%s || %s \n", args, getenv("OLDPWD"));
      perror("cd");
      return 1;
    }
    char* cwd = malloc(sizeof(char)*256);
    if (getcwd(cwd , 256) == NULL ){
      perror("getcwd");
    }
    free(cwd);
    return 0; 
 }
  if (chdir(args) != 0) {
    printf("%s || %s \n", args, getenv("OLDPWD"));
    perror("cd");
    return 1;
  }
  setenv("OLDPWD", getenv("PWD"), 1);  
  return 0;
}


// Execute les pipes dans une commande en avant plan ou en arriere plan
int execPipe(char** args, char* prompt, int nbPipe, int valReturn, works_list *w_list, works* buf, int tmp, pid_t pere, int tube[]) {
	int res = 0;
	pid_t filsAjoute[nbPipe+1]; // les fils que l'on va ajouter au jobs
	//creations des tube qui vont communiquer entre eux pour gerer les sortie des pipes vers le pipe suivant
	int tubePipe[2*nbPipe];
	for(int i = 0; i < nbPipe; i++){
		if(pipe(tubePipe + i*2) < 0) {
				perror("pipe");
				exit(EXIT_FAILURE);
		}
	}
	for(int j=0; j<=nbPipe; j++) {
			char **cmd = parsePipe(args, j);
			pid_t pid = fork();
			if (pid == -1) {
				freeTab(args);
				free(prompt);
				perror("fork");
				exit(EXIT_FAILURE);
			} else if(pid==0) {
				if(j!=0) {
					if(dup2(tubePipe[(j-1)*2], STDIN_FILENO)==-1) {
						perror("dup2");
						exit(EXIT_FAILURE);
					}
				}
				if(j!=nbPipe) {
					if(dup2(tubePipe[(j*2)+1], STDOUT_FILENO)==-1) {
						perror("dup2");
						exit(EXIT_FAILURE);
					}
				} 
				for(int l = 0; l < 2*nbPipe; l++){
					close(tubePipe[l]);
				}
				execute(cmd, prompt, valReturn, w_list, buf, tmp, pere==-1? 0:1);
				exit(valReturn);
			} else {
				if(pere!=-1) {
					filsAjoute[j] = pid;
				}
			}
			freeTab(cmd);
		}
		if(tube!= NULL) {
			if(write(tube[1],filsAjoute, sizeof(pid_t)*(nbPipe+1))==-1) {
				perror("write");
				close(tube[1]);
				close(tube[0]);
			}
		}
		for(int m = 0; m < 2 * nbPipe; m++){
      close(tubePipe[m]);
    }
		for(int n = 0; n < nbPipe + 1; n++) {
			wait(NULL);
		}
		
		return res;
}


int main()
{
	startTerm();
	return 0;
}

int execute (char** args, char* prompt, int valReturn, works_list *w_list, works* buf, int tmp, int arrierePlan){
	if (args == NULL)
		{
			free(prompt);
			exit(valReturn);
		}
		else if (args[0] == NULL)
		{
			free(prompt);
			freeTab(args);
			return valReturn;
		}
		int* fd_sub = NULL;
		int cpt_fd = 0;
		while (there_is_sub(args)){
			if (fd_sub == NULL){
				fd_sub = malloc(sizeof(int));
			}
			fd_sub = realloc(fd_sub, sizeof(int)*(cpt_fd + 1));
			if (fd_sub == NULL)
      {
        perror("realloc");
        exit(EXIT_FAILURE);
      }
			int fd = get_sub(args, prompt, valReturn, w_list, buf, tmp);
			if(fd != -1 ){
				fd_sub[cpt_fd] = fd;
			} else {
				free(prompt);
				freeTab(args);
				exit(EXIT_FAILURE);
			}
			cpt_fd +=1;
		}
		// Debut gestion pour redirection
		//Faire un fic supplementaire pour gerer ca
		// voir la modif de args 
		//Parcourir tout le tableau redir
		int fd_in = -1;
		int fd_out = -1;
		int fd_err = -1;
		int std_out = dup(STDOUT_FILENO);
		int std_in = dup(STDIN_FILENO);
		int std_err_out = dup(STDERR_FILENO);

		int res_containPipe = containsPipe(args);

		if(!res_containPipe) {
			int *redir = containsRedir(args);
			int fd_in = open_file(redir[0], redir[1], args);
			int fd_out = open_file(redir[2], redir[3], args);
			int fd_err = open_file(redir[4], redir[5], args);
			if (redir[0] != -1 || redir[2] != -1 || redir[4] != -1) {	
				modif_args(&args, redir);
				if (fd_in != -1)
				{
					if (redir[0] == 0)
					{
						if (dup2(fd_in, STDIN_FILENO) == -1)
							perror("dup2");
					}
				}
				if (fd_out != -1) {
						if (redir[2] > 0 && redir[2] < 4) {
							if (dup2(fd_out, STDOUT_FILENO) == -1)
								perror("dup2");
						}
				} else {
					if (redir[2] != -1){
						perror("open");
						return 1;
					}
				} 
				if (fd_err != -1) {
					if (redir[4] > 3 && redir[4] < 7) {
						if (dup2(fd_err, STDERR_FILENO) == -1)
							perror("dup2");
					}
				}  else {
					if (redir[4] != -1){
						perror("open");
						return 1;
					}
				} 
			}
			free(redir);
		}
		
		// Fin gestion redirecction de base

		if (strcmp(args[0], "exit") == 0)
		{
			if(stoppedJobInWl(w_list)) {
				free(prompt);
				freeTab(args);
				
				fprintf(stderr, "There are stopped jobs\n");
				return 1;
			}
			free(prompt);
			
			freeAll(w_list);

			if (args[1] != NULL)
			{
				int val = atoi(args[1]);
				freeTab(args);
				exit(val);
			}
			else
			{
				freeTab(args);
				exit(valReturn);
			}
		}
		else if (strcmp(args[0], "cd") == 0)
		{
			
			valReturn = cd(args[1]);
			freeTab(args);
			free(prompt);
		}
		else if (strcmp(args[0], "pwd") == 0 && res_containPipe==0)
		{
			char *cwd = malloc(sizeof(char) * 256);
			if (getcwd(cwd, 256) == NULL)
			{
				perror("getcwd");
			}
			else
			{
				printf("%s\n", cwd);
			}
			free(cwd);
			freeTab(args);
			free(prompt);
		}
		else if (strcmp(args[0], "?") == 0)
		{
			printf("%d\n", valReturn);
			valReturn = 0;
			freeTab(args);
			free(prompt);
		}
		else if (strcmp(args[0], "jobs") == 0)
		{
			valReturn = afficheJobStatusChanged(w_list, 0, args[1] != NULL && strcmp(args[1], "-t") == 0 , valReturn);
			suppressJobsDoneStopped(&w_list);
		}
		else if (strcmp(args[0], "kill") == 0)
		{
			int k;
			int signal;
			int id;
			works *job;
			if (getArgsSize(args) == 2)
			{
				signal = 15;
				if (args[1][0] == '%')
				{
					id = atoi(args[1]+1);
					job = getJobFromId(id, w_list);
					int groupeid = getpgid(job->pid);
					k = kill(-groupeid, signal);
					free(prompt);
					freeTab(args);
					
				}
				else
				{
					int i = atoi(args[1]);
					int groupeid = getpgid(i);
					kill(-groupeid, signal);
					free(prompt);
					freeTab(args);
					
				}
			}
			else if (getArgsSize(args) < 3)
			{
				perror("kill : manque d'arguments");
				freeTab(args);
				free(prompt);
			}
			else
			{
				signal = atoi(args[1]+1);
				if (args[2][0] == '%')
				{
					id = atoi(args[2]+1);
					job = getJobFromId(id, w_list);
					int groupeid = getpgid(job->pid);
					k = kill(-groupeid, signal);
					free(prompt);
					freeTab(args);
					
				}
				else
				{
					id = atoi(args[2]);
					job = getJobFromPid(id, w_list);
					int groupeid = getpgid(job->pid);
					k = kill(-groupeid, signal);
					free(prompt);
					freeTab(args);
					
				}
			
				if (k == -1)
				{
					perror("kill : un ou plusieurs arguments invalides");
					freeTab(args);
					free(prompt);
				}
			}
		}
		else if (strcmp(args[0], "fg") == 0) {
			int id;
			works* job;
			pid_t sauvPid;
			char* sauvNom = malloc(sizeof(char)*256);
			if (getArgsSize(args) == 2) {
				if (args[1][0] == '%') {
					id = atoi(args[1]+1);
					job = getJobFromId(id, w_list);
				} else {
					id = atoi(args[1]);
					job = getJobFromPid(id, w_list);
				}
				if (job->status==STATUS_STOPPED) {
					int groupeid = getpgid(job->pid);
					kill(-groupeid, SIGCONT);
					if (tcsetpgrp(STDIN_FILENO, groupeid)==-1) {
						perror("tcsetpgrp failed 1");
						exit(EXIT_FAILURE);
					}
					sauvPid=job->pid;
					strcpy(sauvNom, job->nom);
					if (waitpid(sauvPid, &tmp, WUNTRACED) == -1) {
						perror("waitpid");
					}
					if(WIFSTOPPED(tmp)) {
						char *aff = affiche_one_job(getJobFromPid(sauvPid, w_list), 0);
						fprintf(stderr, "%s", aff);
						free(aff);
						waitpid(job->pid, NULL, WNOHANG);
					} else {
						changeStatus(sauvPid, STATUS_DONE, w_list);
						suppressJobsDoneStopped(&w_list);
					}
					valReturn = WEXITSTATUS(tmp);
				}
				tcsetpgrp(STDIN_FILENO, getpgrp());
				free(sauvNom);
				freeTab(args);
				free(prompt);
			}
		}
		else if (strcmp(args[0], "bg") == 0) {
			int id;
			works* job;
			if (getArgsSize(args) == 2) {
				if (args[1][0] == '%') {
					id = atoi(args[1]+1);
					job = getJobFromId(id, w_list);
				} else {
					id = atoi(args[1]);
					job = getJobFromPid(id, w_list);
				}
				if (job->status==STATUS_STOPPED || job->status==STATUS_RUNNING) {
					int groupeid = getpgid(job->pid);
					kill(-groupeid, SIGCONT);
				}
			}
		}
		else if(res_containPipe) 
		{
			int nbPipe = res_containPipe;
			int sizeArgs = getArgsSize(args);
			// Si on execute en arriere plan, on va fork puis executer tout les pipe dans le fils
			if(strcmp(args[sizeArgs - 1], "&") == 0) {
				args[sizeArgs - 1] = NULL;

				//creation des tubes qui vont communiquer au pere les fils à ajouter aux jobs
				int tube[2];
				if(pipe(tube) < 0) {
						perror("pipe");
						exit(EXIT_FAILURE);
				}
				pid_t pid = fork();
				if(pid==0) {
					// execution de tout les pipe
					execPipe(args, prompt, nbPipe, valReturn, w_list, buf, tmp, getpid(),tube);
					exit(valReturn);
				} else {
					addWork(w_list, prompt, pid, STATUS_RUNNING);
					char *aff = affiche_one_job(getJobFromPid(pid, w_list), 0);
					fprintf(stderr, "%s", aff);
					free(aff);
					if(waitpid(pid, NULL, WNOHANG)==-1) perror("waitpid3");

					pid_t filsAjoute[nbPipe+1]; // les fils que l'on va ajouter au jobs
					if(read(tube[0], &filsAjoute, sizeof(pid_t)*(nbPipe+1))==-1) {
						perror("read");
					}
					close(tube[0]);
					close(tube[1]);
					for(int i=0; i<=nbPipe; i++) {
						char **cmd = parsePipe(args, i);
						int taille = getArgsSize(cmd);
						char* nom = flatten(cmd, taille);
						addPidSon(w_list, pid, nom, filsAjoute[i], STATUS_RUNNING);
					}
				}
			} else {
				execPipe(args, prompt, nbPipe, valReturn, w_list, buf, tmp, -1, NULL);
			}
			freeTab(args);
			free(prompt);
		}
		else
		{
			int sizeArgs = getArgsSize(args);

			pid_t pid = fork();
			if (pid == -1)
			{
				freeTab(args);
				free(prompt);
				perror("fork");
				exit(EXIT_FAILURE);
			}
			else if (pid == 0)
			{

				setpgid(0,0);
				if (strcmp(args[sizeArgs - 1], "&") == 0)
				{
					args[sizeArgs - 1] = NULL;
				} else {
					if(isatty(STDIN_FILENO) && !arrierePlan) {
						if (tcsetpgrp(STDIN_FILENO, getpgrp())==-1) {
							perror("tcsetpgrp failed 1");
							exit(EXIT_FAILURE);
        		}
					}
				}

				struct sigaction sa;
				memset(&sa, 0, sizeof(struct sigaction));
				sa.sa_handler = SIG_DFL;

				if(sigaction(SIGTSTP, &sa, NULL)==-1) perror("sigaction");
				if(sigaction(SIGINT, &sa, NULL)==-1) perror("sigaction");
				if(sigaction(SIGTERM, &sa, NULL)==-1) perror("sigaction");
				if(execvp(args[0], args)==-1) {
					perror("execvp");
					free(prompt);
					
					freeTab(args);
					exit(valReturn);
				}
				free(prompt);
				
				freeTab(args);
				exit(valReturn);
				
			}
			else
			{
				if (strcmp(args[sizeArgs - 1], "&") == 0)
				{
					addWork(w_list, prompt, pid, STATUS_RUNNING);
					char *aff = affiche_one_job(getJobFromPid(pid, w_list), 0);
					fprintf(stderr, "%s", aff);
					freeTab(args);
					free(prompt);
					free(aff);
					
					waitpid(pid, NULL, WNOHANG);
				}
				else
				{
					if (waitpid(pid, &tmp, WUNTRACED) == -1)
					{
						perror("waitpid");
					}

					if(isatty(STDIN_FILENO)) {
						if (tcsetpgrp(STDIN_FILENO, getpgrp())==-1) {
							perror("tcsetpgrp failed");
							exit(EXIT_FAILURE);
        		}
					}

					if(WIFSTOPPED(tmp)) {
						addWork(w_list, prompt, pid, STATUS_STOPPED);
						char *aff = affiche_one_job(getJobFromPid(pid, w_list), 0);
						fprintf(stderr, "%s", aff);
						free(aff);
						waitpid(pid, NULL, WNOHANG);
					}
					valReturn = WEXITSTATUS(tmp);
					freeTab(args);
					free(prompt);
					
				}
			}
		}
		//Fermeture des eventuelles pipe de substitutions
		if(cpt_fd > 0){
			for (int i = 0; i < cpt_fd; i++)
			{
				close(fd_sub[i]);
			}
			free(fd_sub);
		}
		// On remet toute les entrées et sorties por défault
		if (dup2(std_out, STDOUT_FILENO) == -1)
			perror("dup2");
		if (dup2(std_in, STDIN_FILENO) == -1)
			perror("dup2");
		if (dup2(std_err_out, STDERR_FILENO) == -1)
			perror("dup2");
		// On ferme le fichier
		if (fd_in != -1)
			close(fd_in);
		if (fd_out != -1)
			close(fd_out);
		if (fd_err != -1)
			close(fd_err);
		return valReturn;
}

int startTerm()
{
	int tmp = 0;
	int valReturn = 0;
	works_list *w_list = malloc(sizeof(works_list));
	w_list->job = NULL;
	w_list->next = NULL;
	w_list -> last_id = 0;
	
	while (1)
	{

		valReturn = afficheJobStatusChanged(w_list, 1, 0, valReturn);
		suppressJobsDoneStopped(&w_list);
		//Ignorer ctrl c, v

		struct sigaction sa;
		memset(&sa, 0, sizeof(struct sigaction));
		sa.sa_handler = SIG_IGN;

		if(sigaction(SIGTSTP, &sa, NULL)==-1) perror("sigaction");
		if(sigaction(SIGINT, &sa, NULL)==-1) perror("sigaction");
		if(sigaction(SIGTERM, &sa, NULL)==-1) perror("sigaction");
		if(sigaction(SIGTTIN, &sa, NULL)==-1) perror("sigaction");
		if(sigaction(SIGTTOU, &sa, NULL)==-1) perror("sigaction");
		if(sigaction(SIGQUIT, &sa, NULL)==-1) perror("sigaction");
	
		works *buf = malloc(sizeof(works));

		char *txtprompt = creatTxtPrompt(nb_works_actives(w_list));
		rl_outstream = stderr;
		char *prompt = readline(txtprompt);
		add_history(prompt);
		free(txtprompt);
		char **args = getTableArgs(prompt);

		valReturn = execute(args, prompt, valReturn, w_list, buf, tmp, 0);
		free(buf);

	}
	return 0;
	
}