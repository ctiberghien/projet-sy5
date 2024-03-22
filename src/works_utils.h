#include <sys/types.h>

#ifndef WORKS_UTILS_H
#define WORKS_UTILS_H

#define STATUS_RUNNING 0
#define STATUS_DONE 1
#define STATUS_STOPPED 2
#define STATUS_DETACHED 3
#define STATUS_KILLED 4
#define STATUS_UNKNOWN 42
#define MAX_PATH_LENGTH 256

typedef struct works works;
typedef struct works_list works_list;


struct works {
    char nom[256];
    pid_t pid;
    int status;
    int id;
    int valReturn;
    works_list* sons;
};

struct works_list {
    works* job;
    works_list* next;
    int last_id;
};

// Déclarations de la fonction pour savoir combien de jobs sont en cours
int nb_works_actives(works_list* list);

void addWork(works_list* list, char* nom, pid_t pid, int status);

void freeAll(works_list* wl);

char* affiche_one_job(works* work);

void changeStatus(pid_t pid, int status, works_list* list);

void suppressJobsDoneStopped(works_list** wl);

works* getJobFromId(int i, works_list* wl);

works* getJobFromPid(pid_t pid, works_list* wl);

int getStatusFromSig(int sig);

int stoppedJobInWl(works_list* wl);

int afficheJobStatusChanged(works_list* wl, int err,int is_T ,  int valReturn);

void addPidSon(works_list* list, pid_t pid_father ,char* nom, pid_t pid, int status);

void changeId(int id, works_list* wl);

char* affiche_one_job_t(char* path_file, int pid);

char* affiche_one_job_t_soft(works* job);

#endif
