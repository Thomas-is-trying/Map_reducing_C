#include "mr_log.h"
#include "utilities/mr_protocol_utilities.h"

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <semaphore.h>
#include <string.h>

int mr_log_init(const char *path, mr_log_ctx_t* lctx) {
    //controllo se il puntatore al login ctx è diverso da NULL
    if(lctx == NULL){errno = EINVAL; return -1;}
    //se path è NULL uso il nome di default, richiamando la funzione ricorsivamente
    if(path==NULL){
        if(mr_log_init(LOG_DEFAULT_NAME,lctx)==-1){return -1;};
        return 0;
    }
    else{
        //apro (o creo) il file di log in append, condiviso poi dai processi figli via fork()
        lctx->fd = open(path,O_APPEND | O_CREAT | O_WRONLY,0644);
        if(lctx->fd  == -1){return -1;}

        //salvo l'offset di fine file corrente, per poter rileggere solo gli eventi di questa run
        lctx->run_start_offset = lseek(lctx->fd, 0, SEEK_END);
        if(lctx->run_start_offset == -1){close(lctx->fd); return -1;}

        //costruisco un nome univoco del semaforo per istanza (pid del padre + indirizzo di lctx)
        snprintf(lctx->sem_name, MAX_SIZE_SEM_NAME_LOG, "/mr_log_sem_%d_%p", getpid(), (void*)lctx);

        //creo il semaforo con nome: serve per la mutua esclusione fra processi (mtx_t non basterebbe)
        lctx->semaphore = sem_open(lctx->sem_name,O_CREAT, 0644,1);
        if(lctx->semaphore == SEM_FAILED){close(lctx->fd);return -1;}
    }

    return 0;
}

int mr_log_close(mr_log_ctx_t* lctx) {
   //controllo se il puntatore al login ctx è diverso da NULL
   if(lctx == NULL){errno = EINVAL; return -1;}
   //chiudo solo il riferimento locale: la rimozione definitiva (sem_unlink) resta a mr_destroy
   close(lctx->fd);
   sem_close(lctx->semaphore);
   return 0;
}

int mr_log_write(mr_log_ctx_t* lctx, const char *process_name, const char *event,const char *thread, const char *fmt, ...) {
    //controllo che i vari puntatori siano diversi da NULL
    if(lctx==NULL||process_name==NULL||event==NULL||thread==NULL){errno = EINVAL; return -1;}
    //costruisco il timestamp
    char timestamp[MAX_SIZE_TIME_LOG];
    time_t t = time(NULL);
    struct tm *tmp = localtime(&t);
    if(strftime(timestamp, MAX_SIZE_TIME_LOG, "%x - %X", tmp)== 0){errno = EINVAL;return -1;};

    //espando il messaggio variadico (stile printf) in un buffer locale
    char message[MAX_SIZE_MESSAGE_LOG];
    //dichiaro la lista dlelle variabili
    va_list ap;
    //la popolo con tutte le variabili dopo fmt
    va_start(ap, fmt);
    //snprintf usata specificatamente per messaggi con formato come in questo caso(prende gl iargomenti di va_list)
    vsnprintf(message, sizeof(message), fmt, ap);
    //libero le risorse della va_list
    va_end(ap);

    //compongo la riga finale in un colpo solo con snprintf, più sicuro di strcat multipli(che ho usato in precendeza[to-do cambia])
    char line[MAX_SIZE_LINE_LOG];
    snprintf(line,MAX_SIZE_LINE_LOG,"[%s] [%s] [%s] [%s] [MSG = %s]\n",timestamp,process_name,thread,event,message);

    //scrivo in mutua esclusione fra processi: sem_wait/writen/sem_post

    //occupo il semaforo
    if(sem_wait(lctx->semaphore) == -1){return -1;};
    //riuso la funzione per il protocllo fra processi per scrivere il log
    if(writen(lctx->fd, line, strlen(line)) == -1){sem_post(lctx->semaphore);return -1;};
    //libero il semaforo
    sem_post(lctx->semaphore);
    return 0;
}