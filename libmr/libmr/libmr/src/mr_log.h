#ifndef MR_LOG_H
#define MR_LOG_H

#include <semaphore.h>

//grandezza massima per il nome del semaforo
#define MAX_SIZE_SEM_NAME_LOG  64
//grandezza massima timestamp
#define MAX_SIZE_TIME_LOG  64
//grandezza massima messaggio abbinato al log
#define MAX_SIZE_MESSAGE_LOG  512
//grandezza massima linea  di logging
#define MAX_SIZE_LINE_LOG  1024
//nome default per il log file
#define LOG_DEFAULT_NAME "mr_log.txt"

//Contesto di log di una singola elaborazione
typedef struct {
    //fd del file (aperto in append,condiviso fra i tre processi tramite fork)
    int fd;
    //riferimento al semaforo POSIX nominato usato per la mutua esclusione fra processi
    sem_t *semaphore;
     //il nome del semaforo(serve per poterlo rimuovere con sem_unlink in mr_destroy)
    char sem_name[MAX_SIZE_SEM_NAME_LOG];
    /*l'offset difine file al momento dell'apertura 
    (usato da mr_stats_sniff_log per rileggere solo gli eventi di questa esecuzione, non quelli di run precedentiin append)*/
    off_t run_start_offset;
} mr_log_ctx_t;

/*Funzione per inizializzare il contesto di log puntato da lctx: apre (o crea)
  il file path in append e crea il semaforo nominato per la sincronizzazione
  fra processi. Se path è NULL, usa il nome di default (LOG_DEFAULT_NAME)

  Ritorna 0 dopo aver aperto il file e creato il semaforo con successo
  Ritorna -1 se c'è un problema */
int mr_log_init(const char *path,mr_log_ctx_t* lctx);

/*Funzione per chiudere, per il processo chiamante, il contesto di log puntato
  da lctx: chiude il file descriptor e rilascia il riferimento locale al
  semaforo. Non rimuove il semaforo dal kernel (sem_unlink), responsabilità
  del solo processo principale in mr_destroy,

  Ritorna 0 dopo aver chiuso fd e semaforo
  Ritorna -1 se c'è un problema*/
int mr_log_close(mr_log_ctx_t* lctx);

/*Funzione variadica (stile printf) per scrivere una riga di log nel formato
  "[timestamp] [process_name] [thread] [event] messaggio", sincronizzata fra
  processi tramite il semaforo di lctx

  Ritorna 0 dopo aver scritto la riga con successo
  Ritorna -1 se c'è un problema*/
int mr_log_write(mr_log_ctx_t* lctx, const char *process_name, const char *event,const char *thread, const char *fmt, ...);

#endif