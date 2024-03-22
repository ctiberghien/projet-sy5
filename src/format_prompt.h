#include <stddef.h>


#ifndef FORMAT_PROMPT_H
#define FORMAT_PROMPT_H

// Constantes
#define PROMPT_PATH_MAX_LENGTH 22
#define PROMPT_BUF_LENGTH 256


// Déclarations de la fonction pour generer les information indiqué sur les prompts
char* creatTxtPrompt(int nb_w);

char* change_cwd(char* cwd, char* newCwd, size_t size);

#endif
