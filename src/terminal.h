#ifndef TERMINAL_H
#define TERMINAL_H

int startTerm();
int execute (char** args, char* prompt, int valReturn, works_list *w_list, works* buf, int tmp, int arrierePlan);
int cd(char* args);

#endif
