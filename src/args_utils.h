#ifndef ARGS_UTILS
#define ARGS_UTILS

#include "works_utils.h"


#define STANDARD_IN "<"
#define STANDARD_OUT ">"
#define STANDARD_OUT_CONC ">>"
#define STANDARD_OUT_CRUSH ">|"
#define ERR_OUT "2>"
#define ERR_OUT_CONC "2>>"
#define ERR_OUT_CRUSH "2>|"
#define PIPE "|"
#define SUB "<("


void freeTab(char** tab);
int getArgsSize(char** args);
char** getTableArgs (char* args);
int* containsRedir(char** args);
int open_file(int redir, int name_pose, char** args);
void modif_args(char*** args, int* redir);
int containsPipe(char **args);
char** parsePipe(char **args, int nbPipe);
int there_is_sub (char** args);
int get_sub(char ** args, char* prompt, int valReturn, works_list *w_list, works* buf, int tmp);
void clean_args_def (char** args);
void clean_args (char*** args);
char* flatten(char **args, int taille);

#endif
