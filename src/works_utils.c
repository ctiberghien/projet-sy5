#include "works_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <wait.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

int nb_works_actives(works_list* list){

  works_list* act = list;
  int i= 0;
  while (act != NULL){
    if (act -> job != NULL ){
      if( (act -> job -> status) != STATUS_DONE){	
        i +=1;
      }
    }
    act = act -> next;
  }
  return i;

}

//Modifier pour faire list une adresse et pas la liste
void addWork(works_list* list, char* nom, pid_t pid, int status){
  works_list* act = list;
  works* tmpW = malloc(sizeof(works));
  int new_id = (list -> last_id) +1;
  list -> last_id =  new_id;
  strcpy(tmpW -> nom, nom);
  tmpW -> pid = pid;
  tmpW -> status = status;
  tmpW -> id = new_id;
  tmpW -> sons = NULL;
  if (list -> job == NULL){
    list -> job = tmpW;
    list -> last_id =  new_id;
  } else {
    while (act -> next != NULL){
      act -> last_id = new_id;
      act = act -> next;
    }
    works_list* tmp = malloc(sizeof(works_list));
    tmp -> job = tmpW;
    tmp -> next = NULL;  
    act -> next = tmp;
  }
}

void freeAll(works_list* wl) {
  //TODO Test
  if (wl -> next == NULL){
    if (wl -> job != NULL)
      {
	if(wl -> job -> sons != NULL)
	  {
	    freeAll(wl -> job -> sons);
	  }
	free(wl -> job);
      }
    free(wl);
  } else {
    freeAll(wl -> next);    
  }
}

void changeStatus(pid_t pid, int status, works_list* list){
  if(status == STATUS_DONE ||
    status == STATUS_RUNNING ||
    status == STATUS_STOPPED || 
    status == STATUS_DETACHED ||
    status == STATUS_KILLED){
    works_list* act = list;
    while(act  != NULL){
      if (act -> job != NULL && act -> job -> pid == pid){
        act -> job -> status = status;
        return;
      } else {
        act = act -> next;
      }
    }
  }
}



void suppressJobsDoneStopped(works_list** head){
  //TODO Modify with new struct (ne pas supprimer si il y as des sons et supprmier que si il a tout les sons qui sont finis
  works_list* current = *head;
  works_list* prev = NULL;
  int id = current -> last_id;
  while (current != NULL) {
    if (current->job != NULL &&
	(current->job->status == STATUS_DONE || current->job->status == STATUS_KILLED)) {
      if (current->job != NULL && current->job -> sons != NULL)
        suppressJobsDoneStopped(& (current->job -> sons));
      works_list* temp = current;
      if (prev != NULL) {
	prev->next = current->next;
      } else {
	*head = current->next;
      }
      current = current->next;
      free(temp->job);
      free(temp);
    } else {
      prev = current;
      current = current->next;
    }
  }
  
  // Ajouter un nœud avec job NULL si la liste est vide
  if (*head == NULL) {
    *head = malloc(sizeof(struct works_list));
    (*head)->job = NULL;
    (*head)->next = NULL;
    (*head)->last_id = id;
  }
}

char*  affiche_one_job(works* job){
  //[1]  590622  Done sleep 1000
  char* res = malloc(sizeof(char)*512);
  int size = 0;
  res[0]='\0';
  strcat(res, "[");
  if (job -> id > 10){
    char* str = malloc(sizeof(char)*12);
    sprintf(str, "%d", job -> id);
    strcat(res, str);
    strcat(res, "] ");
    size +=4;
    free(str); 
  } else {
    char* str = malloc(sizeof(char)*12);
    sprintf(str, "%d", job -> id);
    strcat(res, str);
    strcat(res, "]  ");
    size +=3;
    free(str);
  }
  char* pid = malloc(sizeof(char)*256);
  sprintf(pid, "%d", job -> pid);
  int size_pid = strlen(pid);
  strcat(res, pid);
  size += size_pid;
  while (size < 15){
    size +=1;
    strcat(res, " ");
  }
  free(pid);
  if (job -> status == STATUS_RUNNING){
    strcat(res, "Running   ");
  } else  if (job -> status == STATUS_DONE){
    strcat(res, "Done      ");
  } else if (job -> status == STATUS_STOPPED){
    strcat(res, "Stopped   ");
  } else if (job -> status == STATUS_KILLED){
    strcat(res, "Killed    ");
  } else if (job -> status == STATUS_DETACHED){
    strcat(res, "Detached  ");
  }else {
    exit(EXIT_FAILURE);
  }
  size+=10;

  //Suprimer le & a la fin
  if (*(job-> nom + strlen(job -> nom) -1) == '&'){
    strncat(res, job->nom, strlen(job->nom) - 2);
  } else {
    strcat(res, job->nom);
  }
  
  strcat(res, "\n\0");
  return res;
}

char* affiche_one_job_t_soft(works* job){
  char* res = malloc(sizeof(char)*512);
  int size = 0;
  res[0] = ' ';
  res[1] = ' ';
  res[2] = '\0';
  
  char* pid = malloc(sizeof(char)*256);
  sprintf(pid, "%d", job -> pid);
  int size_pid = strlen(pid);
  strcat(res, pid);
  size += size_pid;
  while (size < 15){
    size +=1;
    strcat(res, " ");
  }
  free(pid);
  if (job -> status == STATUS_RUNNING){
    strcat(res, "Running   ");
  } else  if (job -> status == STATUS_DONE){
    strcat(res, "Done      ");
  } else if (job -> status == STATUS_STOPPED){
    strcat(res, "Stopped   ");
  } else if (job -> status == STATUS_KILLED){
    strcat(res, "Killed    ");
  } else if (job -> status == STATUS_DETACHED){
    strcat(res, "Detached  ");
  }else {
    exit(EXIT_FAILURE);
  }
  size+=10;

  //Suprimer le & a la fin
  if (*(job-> nom + strlen(job -> nom) -1) == '&'){
    strncat(res, job->nom, strlen(job->nom) - 2);
  } else {
    strcat(res, job->nom);
  }
  
  strcat(res, "\n\0");
  return res;

}

//proc/<pid>/comm
///proc/<pid>/task/<tid> (dossier)/children
char* affiche_one_job_t(char* path_file, int pid){
  //Version /proc qui ne marche pas
  char path_sons[MAX_PATH_LENGTH];
  snprintf(path_sons, MAX_PATH_LENGTH, "%s/task/%d/children", path_file, pid);
  int fd_sons = -1;
  fd_sons = open(path_sons, O_RDONLY);
  if (fd_sons == -1) perror("open");
  char pid_char[42];
  ssize_t bytesRead;
  int i = 0;
  while ((bytesRead = read(fd_sons, pid_char, 1)) > 0) {
    if(pid_char[i] == '\n'){
      pid_char[i] = '\0';
      char new_path[MAX_PATH_LENGTH];
      snprintf(new_path, MAX_PATH_LENGTH, "/proc/%s/task/", pid_char);
      affiche_one_job_t(new_path, atoi(pid_char));
    }
    i+=1;   
  }
  close(fd_sons);

  char path_name[MAX_PATH_LENGTH];
  snprintf(path_name, MAX_PATH_LENGTH, "%s/comm", path_file);
  int fd_name = -1;
  fd_name = open(path_name, O_RDONLY);
  if (fd_name == -1) perror("open");
  char name_char[256];
  if(read(fd_name, name_char, 255) < 0)
    perror("open");

  char state;
  char path_status[MAX_PATH_LENGTH];
  snprintf(path_status, sizeof(path_status), "%s/stat", path_file);
  FILE *file = fopen(path_status, "r");
  if (file == NULL) {
    perror("Erreur lors de l'ouverture du fichier stat");
    exit(EXIT_FAILURE);
  }
  if(fscanf(file, "%*d %*s %c", &state) == -1) perror("fscanf");
  fclose(file);

  printf("%s", name_char);
  char* ligne = malloc(sizeof(char) * MAX_PATH_LENGTH);
  if(scanf(ligne, "%d       %c   %s\n", pid, state, name_char) == -1) perror("scanf");
  return ligne;
}

works* getJobFromId(int i, works_list* wl) {
  works_list* list = wl;
  while(list->next!=NULL) {
    if (list -> job != NULL && list->job->id==i) {
      return list->job;
    }
    list=list->next;
  }
  if (list->job->id==i) {
    return list->job;
  } else {
    return NULL;
  }
}


works* getJobFromPid(pid_t pid, works_list* wl) {
  works_list* list = wl;
  while(list->next!=NULL) {
    if (list->job->pid==pid) {
      return list->job;
    }
    list=list->next;
  }
  if (list->job->pid==pid) {
    return list->job;
  } else {
    return NULL;
  }
}

int getStatusFromSig(int sig){
  if (sig == 15 || sig ==9 || sig==14) return STATUS_KILLED;
  if (sig == 1) return STATUS_DETACHED;
  if (sig ==  18 || sig == 10 || sig == 12) return STATUS_RUNNING;
  if (sig==20) return STATUS_STOPPED;
  if (sig==19) return STATUS_RUNNING;
  return STATUS_UNKNOWN;
}

int stoppedJobInWl(works_list* wl) {
  works_list* tmp = wl;
  while(tmp!=NULL) {
    if(tmp->job==NULL) break;
    if(tmp->job->status == STATUS_STOPPED) {
      return 1;
    }
    tmp=tmp->next;
  }
  return 0;

}

//permet l'affichage des jobs qui ont changé de status et aussi de jobs
int afficheJobStatusChanged(works_list* wl, int err, int is_T, int valReturn) {
  FILE* affichage = err==1 ? stderr : stdout; 
  works_list* tmp = wl;
  int status = 0;
  int test;
  int res = valReturn;
  while(tmp!=NULL) {
    if(tmp->job==NULL) break;
    if((test = waitpid(tmp->job->pid, &status, WNOHANG | WUNTRACED | WCONTINUED))==-1) perror("waitpid2");
    if(WIFCONTINUED(status)) {
      changeStatus(tmp->job->pid, STATUS_RUNNING, wl);
      char* aff = affiche_one_job(tmp->job);
      fprintf(affichage, "%s",aff);
      free(aff);
    } 
    else if(WIFSTOPPED(status) && test != 0) {
      changeStatus(tmp->job->pid, STATUS_STOPPED, wl);
      char* aff = affiche_one_job(tmp->job);
      fprintf(affichage, "%s", aff);
      free(aff);
    }
    else if(WIFSIGNALED(status) && test !=0) {
      changeStatus(tmp->job->pid, getStatusFromSig(WTERMSIG(status)), wl);
      char* aff = affiche_one_job(tmp->job);
      fprintf(affichage, "%s", aff);
      free(aff);
    }
    else if(WIFEXITED(status) && test!=0) {
      changeStatus(tmp->job->pid, STATUS_DONE, wl);
      char* aff = affiche_one_job(tmp->job);
      fprintf(affichage, "%s", aff);
      free(aff);
      res = WEXITSTATUS(status);
    }
    else if(err==0) {
      char* aff = affiche_one_job(tmp->job);
      fprintf(affichage, "%s", aff);
      free(aff);
      if (is_T)
      {
        if(tmp->job->sons!=NULL) {
          works_list* tmp2 = tmp->job->sons;
          while(tmp2!=NULL) {
            char* aff2 = affiche_one_job_t_soft(tmp2 -> job);
            fprintf(affichage, "%s", aff2);
            free(aff2);
            tmp2=tmp2->next;
          }
        }
      }
    }
    tmp=tmp->next;
    status=0;
  }
  return res;
}

void addPidSon(works_list* list, pid_t pid_father ,char* nom, pid_t pid, int status)
{
  if (list == NULL ) perror("addPidSon");
  works_list* act = list;
  //On creait la structure works_list pour le mettre dans les sons
  works* new_son = malloc(sizeof(works));
  strcpy(new_son -> nom, nom);
  new_son -> pid = pid;
  new_son -> status = status ;
  new_son -> id = list -> last_id;
  new_son -> sons = NULL;
  //On parcour la list pour trouver le bon noeud avec le pid
  while (act -> next != NULL)
    {
      //Si on trouve le bon on s'arrete
      if ( act -> job != NULL && act -> job -> pid == pid_father)
      {
        break;
      }
      act = act -> next;
    }
  //On verifie que se soit bien le bon (si c'etait le dernier de la liste nécéssaire)
  if ( act -> job == NULL || act -> job -> pid != pid_father)
    {
      perror("addPidSon");
    }
  //Si il n'a pas de sons alors on met directement
  if (act -> job -> sons == NULL)
    {
      works_list* tmp = malloc(sizeof(works_list));
      tmp -> job = new_son;
      tmp -> next = NULL;
      act -> job -> sons = tmp;
    }
  else 
    {
      //On parcours la list de son et ajoute le son
      works_list* act_sons = act->job->sons;
      while (act_sons -> next != NULL)
	{
	  //On cherche la fin
	  act_sons = act_sons -> next;
	}
      //On met a la fin le nouveau fils
      works_list* tmp = malloc(sizeof(works_list));
      tmp -> job = new_son;
      tmp -> next = NULL;
      act_sons -> next = tmp;
    }
  list -> last_id = new_son -> id;    
}

void changeId(int id, works_list* wl) {
  works_list* list = wl;
  while (list->next!=NULL) {
    list=list->next;
  }
  list->job->id=id;
}

