#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "args_utils.h"
#include "works_utils.h"
#include "terminal.h"

int getArgsSize(char **args)
{
  int i = 0;
  while (args[i] != NULL)
  {
    i += 1;
  }
  return i;
}

void freeTab(char **tab)
{
  if (tab != NULL)
  {
    int i = 0;
    while (tab[i] != NULL){
      free(tab[i]);
      i+=1;
    }
    free(tab);
  }
}

char **getTableArgs(char *args)
{
  if (args == NULL)
  {
    return NULL;
  }
  else if (*args == '\n')
  {
    char **tabArgs = malloc(sizeof(char *));
    tabArgs[0] = "";
    return tabArgs;
  }
  char **tabArgs = NULL;
  size_t tabSize = 1;

  char *cpyArgs = strdup(args);
  if (cpyArgs == NULL)
  {
    perror("strdup");
    free(cpyArgs);
    exit(EXIT_FAILURE);
  }
  char *token = strtok(cpyArgs, " ");
  while (token != NULL)
  {
    tabArgs = realloc(tabArgs, tabSize * sizeof(char *));
    if (tabArgs == NULL)
    {
      perror("realloc");
      free(cpyArgs);
      free(token);
      freeTab(tabArgs);
      exit(EXIT_FAILURE);
    }
    tabArgs[tabSize - 1] = strdup(token);
    if (tabArgs[tabSize - 1] == NULL)
    {
      perror("strdup");
      free(cpyArgs);
      freeTab(tabArgs);
      exit(EXIT_FAILURE);
    }
    tabSize += 1;
    token = strtok(NULL, " ");
  }
  free(cpyArgs);
  free(token);
  tabArgs = realloc(tabArgs, tabSize * sizeof(char *));
  if (tabArgs == NULL)
  {
    perror("realloc");
    freeTab(tabArgs);
    exit(EXIT_FAILURE);
  }
  tabArgs[tabSize - 1] = NULL;
  return tabArgs;
}

int* containsRedir(char **args)
{
  int cpt = 0;
  int* res = malloc(sizeof(int)* 6);
  for (int i = 0; i < 6; i++) res[i] = -1;
  //[typeIn, posIn , typeOut, posOut, typeErr, posErr]
  res[0]= -1;
  while (args[cpt] != NULL)
  {
    if (strcmp(args[cpt], STANDARD_IN) == 0)
    {
      res[0] = 0; 
      res[1] = cpt;  
    }
    else if (strcmp(args[cpt], STANDARD_OUT) == 0)
    {
      res[2] = 1;
      res[3] = cpt;  
    }
    else if (strcmp(args[cpt], STANDARD_OUT_CONC) == 0)
    {
      res[2] = 2;
      res[3] = cpt;
    }
    else if (strcmp(args[cpt], STANDARD_OUT_CRUSH) == 0)
    {
      res[2] = 3;
      res[3] = cpt;
    }
    else if (strcmp(args[cpt], ERR_OUT) == 0)
    {
      res[4] = 4;
      res[5] = cpt;
    }
    else if (strcmp(args[cpt], ERR_OUT_CONC) == 0)
    {
      res[4] = 5;
      res[5] = cpt;
    }
    else if (strcmp(args[cpt], ERR_OUT_CRUSH) == 0)
    {
      res[4] = 6;
      res[5] = cpt;
    }
    cpt++;
  }
  return res;
}

int open_file(int redir, int pos_name, char** args){
  if (redir == -1) return -1;
  int flags;
  int mods = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
  if(redir == 0) flags = O_RDONLY;
  if (redir == 2 || redir == 5) flags = flags | O_APPEND;
  if (redir == 3 || redir == 6) flags = flags | O_TRUNC;
  if (redir == 1 || redir == 4) flags = flags | O_EXCL;
  if (redir > 0 ) {
    flags = flags | O_CREAT | O_WRONLY;
    return open(args[pos_name+1], flags, mods);
  }
  return open(args[pos_name+1], flags);

}

void modif_args(char*** args, int* redir){
  int ind = 42;
  int pos_in = redir[1] == -1 ? 12000 : redir[1];
  int pos_out = redir[3] == -1 ? 12000 : redir[3];
  int pos_err = redir[5] == -1 ? 12000 : redir[5];
  if (pos_in < pos_out && pos_in < pos_err) ind = pos_in;
  if (pos_out < pos_in && pos_out < pos_err) ind = pos_out;
  if (pos_err < pos_out && pos_err < pos_in) ind = pos_err;
  if(*args == NULL || ind < 0) return;
  int args_size = getArgsSize(*args);
  if(ind > args_size) return;

  for (int i = ind; (*args)[i] != NULL; i++)
  {
    free((*args)[i]);
  }
  char** new_array = realloc(*args, (ind + 1) * sizeof(char*));
  if (new_array != NULL) {
    *args = new_array;
  }
  (*args)[ind] = NULL;
}

int containsPipe(char **args) {
  int cpt=0;
  int res=0;
  while(args[cpt]!=NULL) {
    if(strcmp(args[cpt], "|")==0) {
      res+=1;
    }
    cpt+=1;
  }
  return res;
}

char** parsePipe(char **args, int nbPipe) {
  char** res = malloc(sizeof(char*));
  int cpt = 0;
  if(nbPipe!=0) {
    int cptPipe=0;
    while(cptPipe!=nbPipe) {
      if(strcmp(args[cpt],"|")==0) {
        cptPipe+=1;
      }
      cpt+=1;
    }
  }
  int resSize = 1;
  while(args[cpt]!=NULL && strcmp(args[cpt],"|")!=0) {
    res = realloc(res, sizeof(char*)*resSize);
    if (res == NULL) {
      freeTab(res);
      exit(EXIT_FAILURE);
    }
    res[resSize-1] = strdup(args[cpt]);
    resSize+=1;
    cpt+=1;
  }
  res = realloc(res, resSize * sizeof(char *));
  if (res == NULL)
  {
    perror("realloc");
    freeTab(res);
    exit(EXIT_FAILURE);
  }
  res[resSize - 1] = NULL;
  return res;
}

int there_is_sub (char** args){
  int res = 0;
  int res2 = 0;
  int cpt = 0;
  while (args[cpt]!= NULL){
    if (strcmp(args[cpt], "<(") == 0){
      res +=1;
      res2 +=1;
    }
    if (strcmp(args[cpt], ")") == 0){
      res -=1;
      res2 +=1;
    }
    cpt +=1;
  }
  return res == 0 && res2 != 0;
}

int get_nbr_sub_imbrique(char ** args){
  int cpt1 = 0;
  int nbr_sub_o = 0;
  int nbr_sub_f = 0;
  int nbr_sub_imbr = 0;
  while (args[cpt1]!= NULL ){
    if (strcmp(args[cpt1], "<(") == 0){
      nbr_sub_o +=1;
    }
    if (strcmp(args[cpt1], ")") == 0){
      nbr_sub_f +=1;
    }
    if(nbr_sub_f == nbr_sub_o && nbr_sub_f != 0){
      nbr_sub_imbr = nbr_sub_o;
      break;
    }
    cpt1 +=1;
  }
  return nbr_sub_imbr;
}

int get_dbt_sub (char** args){
  int cpt_dbt = 0;
  while (args[cpt_dbt]!= NULL ){
    if (strcmp(args[cpt_dbt], "<(") == 0)
    {
      break;
    }
    cpt_dbt +=1;
  }
  return cpt_dbt;
}

int get_size_sub(char** args,int cpt_dbt, int nbr_sub_imbr){
  int tabSize = 0;
  while (args[cpt_dbt + tabSize]!= NULL ){
    if (strcmp(args[cpt_dbt + tabSize], ")") == 0)
    {
      nbr_sub_imbr -=1;
      if (nbr_sub_imbr == 0)
      {
        tabSize -= 1;
        break;
      }      
    }
    tabSize +=1;
  }
  return tabSize;
}

char** getTabSub(char **args, int tabSize, int cpt_dbt){
  char **tabJobsSubs = NULL;
  tabJobsSubs = malloc(sizeof(char*)* (tabSize + 1));
  if(tabJobsSubs == NULL) {perror("malloc");exit(EXIT_FAILURE);}
  for (int i = 0; i < tabSize; i++)
  {
    tabJobsSubs[i] = strdup(args[cpt_dbt + i + 1]);
    if(tabJobsSubs[i] == NULL){
      perror("strdup");
      freeTab(tabJobsSubs);
      exit(EXIT_FAILURE);
    }
  }
  tabJobsSubs[tabSize] = NULL;
  for(int j = 0; j < tabSize+2; j++) {
    free(args[cpt_dbt + j]);
    args[cpt_dbt + j] = strdup("");
    if(args[cpt_dbt + j] == NULL){
      perror("strdup");
      freeTab(tabJobsSubs);
      exit(EXIT_FAILURE);
    }
  }
  return tabJobsSubs;
}

int get_sub(char ** args, char* prompt, int valReturn, works_list *w_list, works* buf, int tmp){
  if (args == NULL)
  {
    return -1;
  }
  int nbr_sub_imbr = get_nbr_sub_imbrique(args);

  int cpt_dbt = get_dbt_sub(args);
  int tabSize = get_size_sub(args, cpt_dbt, nbr_sub_imbr);
  
  char **tabJobsSubs = getTabSub(args, tabSize, cpt_dbt);
  

  int tube[2];
  if(pipe(tube) == -1){
    perror("pipe");
    exit(EXIT_FAILURE);
  }
  pid_t pid = fork();
  if(pid == -1) {
		freeTab(args);
		free(prompt);
    perror("fork");
		exit(EXIT_FAILURE);
	} else if (pid==0) {
    if (dup2(tube[1], STDOUT_FILENO) == -1) {
      perror("Erreur lors de la redirection de la sortie standard");
      exit(EXIT_FAILURE);
    }
    execute(tabJobsSubs, prompt, valReturn, w_list, buf, tmp, 0);
    close(tube[0]);
    close(tube[1]);
    exit(EXIT_SUCCESS);
  } else {
    wait(NULL);
    freeTab(tabJobsSubs);
    char* path = malloc(sizeof(char)*42);
    sprintf(path, "/proc/self/fd/%d", tube[0]);
    free(args[cpt_dbt]);
    args[cpt_dbt] = strdup(path);
    free(path);
    if(args[cpt_dbt] == NULL){
      perror("strdup");
      freeTab(tabJobsSubs);
      exit(EXIT_FAILURE);
    }
    clean_args_def(args);
    close(tube[1]);
    return tube[0];
  }
}

void clean_args_def (char** args){
  int i = 0;
  int nb_a_free = 0;
  while (args[i] != NULL) {
    if (strcmp(args[i], "") == 0) {
      free(args[i]);
      for (int j = i; args[j] != NULL; j++) {
        args[j] = args[j + 1];
      }
    nb_a_free += 1;
    } else {
      i++;
    }
  } 
}

char* flatten(char **args, int taille) {
  
  int taille_totale = 0;
  for (int i = 0; i < taille; i++) {
    taille_totale += strlen(args[i])+1;
  }

  char* res = (char*)malloc(taille_totale + 1); 

  if (res == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  int position = 0;
  for (int i = 0; i < taille; i++) {
    strcpy(res + position, args[i]);
    position += strlen(args[i]);
    strcpy(res+position, " ");
    position++;
  }

  return res;
}