#ifndef QUEUE_H
#define QUEUE_H

#include <threads.h>
#include <stddef.h>

/*
 * Coda produttore-consumatore generica a capacità fissa, usata dai
 * thread C11 interni al processo mapper e al processo reducer (sez. 6
 * della specifica) secondo lo schema produttore-consumatore.
 *
 * Non è una pipe e non è memoria condivisa fra processi: è una
 * normale struttura dati accessibile solo dai thread dello stesso
 * processo, protetta da mtx_t e cnd_t.
 *
 * Contiene puntatori void* agli elementi: chi usa la coda decide cosa
 * ci metta dentro (es. una riga deserializzata, una coppia
 * <token,value>) e resta responsabile di allocare/liberare la memoria
 * puntata da ogni elemento — la coda si limita a farli transitare fra
 * producer e consumer.
 */
typedef struct {
    void **items;
    size_t capacity;
    size_t count;
    size_t head;
    size_t tail;

    int closed; /* 1 quando non arriveranno più elementi */

    mtx_t lock;
    cnd_t not_empty;
    cnd_t not_full;
} mr_queue_t;

/*Funzione per inizializzare mr_queue_t con capacity(numero massimo di elementi),
  Ritorna 0 dopo aver settato tutti i campi di mr_queue_t
  Ritorna -1 se c'è un problema*/
int mr_queue_init(mr_queue_t *q, size_t capacity);

/*Funzione per liberare mr_queue_t, chiamata quando tutti i thread hanno già finito,
  NON libera la memoria puntata dagli eventuali elementi, dato che quando viene chiamata la coda dovrebbe essere vuota(gli elementi liberati dai worker)
  Ritorna 0 dopo aver liberato tutti i campi di mr_queue_t
  Ritorna -1 se c'è un problema*/
int mr_queue_destroy(mr_queue_t *q);

/*Funzione per push su mr_queue_t di item,
 Blocca se la coda è piena, finché non c'è spazio. Ritorna sempre 0. */
int mr_queue_push(mr_queue_t *q, void *item);

/*
   Funzione per pop su mr_queue_t di item,
   Blocca se la coda è vuota e non ancora chiusa.
   Ritorna 0 e imposta *item se un elemento è stato estratto,
   ritorna 1 (senza toccare *item) se la coda è vuota ED è stata chiusa: segnale di fine flusso, nessun altro elemento arriverà.
 */
int mr_queue_pop(mr_queue_t *q, void **item);

/*
   Funzione per segnalare che nessun nuovo elemento arrivera in mr_queue_t ,
   sveglia tutti i consumatori eventualmente bloccati in attesa su una coda vuota,
   così possono verificare la condizione di chiusura ed uscire.
   Non svuota la coda: gli elementi già presenti restano
   disponibili e vanno comunque estratti con mr_queue_pop() prima che
   questa ritorni 1.
   Ritorna -1 se c'è un problema
 */
int mr_queue_close(mr_queue_t *q);

#endif