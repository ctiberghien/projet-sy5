#include "format_prompt.h"


#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "works_utils.h"


//char* change_cwd(char* cwd, char* newCwd, size_t size);


char* creatTxtPrompt(int nb_w){
  //Couleur revoir
  char* res = malloc(sizeof(char)*256);
  res[0] = '\0';
  char* color1 = "\001\e[0;31m\002";
  char* color2 = "\001\e[0;32m\002";
  char* colorReset = "\001\e[0;37m\002";
  char* cwd = malloc(PROMPT_BUF_LENGTH*sizeof(char));
  strcat(res, color1);
  int nb = nb_w;
  char* jobs = malloc(sizeof(char)*2);
  char* newCwd = malloc(PROMPT_BUF_LENGTH*sizeof(char));
  sprintf(jobs,"%d",nb);
  strcat(res, "[");
  strcat(res, jobs);
  strcat(res, "]");
  strcat(res, color2);
  int flag=0;
  if(getcwd(cwd, PROMPT_BUF_LENGTH)== NULL){
    perror ("getcwd() error");
  }
  if (strlen(cwd) > PROMPT_PATH_MAX_LENGTH ) {
    newCwd = change_cwd(cwd, newCwd, strlen(cwd));
    flag=1;
  }
  if (flag==1) strcat(res, newCwd);
  else strcat(res, cwd);
  strcat (res, colorReset);
  strcat(res, "$ ");
  free(jobs);
  free(cwd);
  free(newCwd);
  return res;
}


char* change_cwd(char* cwd, char* newCwd, size_t size){
  newCwd[0] = '.';
  newCwd[1] = '.';
  newCwd[2] = '.';
  for (int i = 0 ; i < PROMPT_PATH_MAX_LENGTH ; i +=1){
    newCwd[i+3] = cwd[size - PROMPT_PATH_MAX_LENGTH +i];
  }
  newCwd[PROMPT_PATH_MAX_LENGTH +3] = '\0';
  return newCwd;
}
