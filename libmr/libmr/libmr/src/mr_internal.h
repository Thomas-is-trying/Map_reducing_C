#ifndef MR_INTERNAL_H
#define MR_INTERNAL_H

#include "../include/mr.h"
#include "mr_log.h"
#include "utilities/mr_stats_utilities.h"
#include <sys/types.h>

/*
 * Struttura interna opaca all'esterno (mr_t e' un puntatore a questa
 * struct, definita solo qui e mai esposta in mr.h). Contiene lo stato
 * condiviso di una singola elaborazione. Requisito "istanze multiple
 * senza interferenze" (addendum): niente variabili globali o static
 * per-elaborazione, tutto qui dentro o in strutture raggiungibili
 * solo tramite il puntatore mr_t (unica eccezione: il nome del
 * semaforo POSIX del log, reso univoco per istanza combinando
 * getpid() e l'indirizzo di mr_log_ctx, vedi mr_log.c).
 *
 * pid dei figli e fd delle pipe NON sono qui: restano variabili
 * locali di mr_start, perche' quella funzione fa tutto in una singola
 * chiamata sequenziale/bloccante e nessun altro codice ne ha bisogno.
 * mapper_ctx_t, reducer_ctx_t e worker_reducer_ctx_t (i contesti
 * condivisi fra i thread interni a un singolo processo mapper/reducer)
 * restano invece definiti localmente in mapper_process.c e
 * reducer_process.c: dettagli implementativi di un solo processo, non
 * stato condiviso dell'elaborazione.
 */
struct mr {

    /* copia profonda di attr, fatta da mr_create tramite
       mr_attr_utilities: il chiamante puo' distruggere/modificare la
       propria mr_attr_t dopo la chiamata senza alterare l'elaborazione */
    mr_attr_t attr;

    /* callback mapper/reducer fornite dall'utente a mr_create */
    mr_mapper_t mapper;
    mr_reducer_t reducer;

    /* passato senza interpretazione alle funzioni mapper e reducer */
    void *user_arg;

    /* contesto del log, condiviso fra i tre processi tramite fork()
       (aperto una sola volta, prima dei due fork, dentro mr_start) */
    mr_log_ctx_t mr_log_ctx;

    /* statistiche finali, popolate al termine di mr_start tramite
       mr_stats_sniff_log: mapper e reducer, essendo processi separati,
       non possono scrivere direttamente qui (dopo fork() ogni processo
       ha una copia privata della memoria), quindi registrano i propri
       contatori nel log e il processo principale li rilegge da li'
        + timer per elapsed_seconds
        + variabile nel main per lines_read*/
    mr_stats_t stats;
};


/*Funzione eseguita nel processo figlio mapper dopo la fork() (chiamata da mr_start
  nel ramo figlio, dopo dup2 di stdin/stdout sulle pipe corrette): crea al suo interno
  i thread C11 necessari (un thread reader che legge righe da stdin, mr->attr.mapper_threads
  thread worker che invocano mr->mapper e producono coppie), attende la loro terminazione
  e chiude la pipe verso il reducer una sola volta, dal codice di coordinamento, solo dopo
  che tutti i thread worker sono terminati . Non fa parte dell'interfaccia pubblica;
  non ritorna nulla, il processo figlio termina con _exit()
  subito dopo questa chiamata*/
void mapper_process_main(struct mr *mr);

/*Funzione eseguita nel processo figlio reducer dopo la fork() (chiamata da mr_start
  nel ramo figlio, dopo dup2 di stdin/stdout sulle pipe corrette): un thread reader
  raggruppa le coppie ricevute per token, poi il coordinamento smista ogni gruppo
  completo verso una delle mr->attr.reducer_threads liste in base a hash(token), i
  thread worker elaborano ciascuno la propria lista invocando mr->reducer una sola
  volta per gruppo, i risultati vengono accumulati e infine scritti in ordine
  lessicografico per token verso il processo principale. Non fa parte dell'interfaccia
  pubblica; non ritorna nulla, il processo figlio termina con _exit() subito dopo
  questa chiamata*/
void reducer_process_main(struct mr *mr);

#endif