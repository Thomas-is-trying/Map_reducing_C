#include "mr_queue_utilities.h"
#include <stdlib.h>
#include <errno.h>


int mr_queue_init(mr_queue_t *q, size_t capacity) {
    //controllo che la capacity e il puntatore siano validi
    if (capacity == 0 || q == NULL) {errno = EINVAL; return -1;}
    //inizializzo un array di puntatori a puntatori di tipo void che poi userò con mr_file_line_t
    q->items = (void **)malloc(capacity * sizeof(void *));
    if (q->items == NULL) {return -1;}
    //assegno i restanti atributi
    q->capacity = capacity;
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    q->closed = 0;

    /*mtx_init/cnd_init con mtx_plain: nessuna ricorsione o timeout */
    mtx_init(&q->lock, mtx_plain);
    cnd_init(&q->not_empty);
    cnd_init(&q->not_full);

    return 0;
}

int mr_queue_destroy(mr_queue_t *q) {
    if (q == NULL) {errno = EINVAL; return -1;}
    //libera il guscio dell'array, gli elementi vengono liberati dai worker
    free(q->items);
    //libero lock e condition variables
    mtx_destroy(&q->lock);
    cnd_destroy(&q->not_empty);
    cnd_destroy(&q->not_full);
    return 0;
}

int mr_queue_push(mr_queue_t *q, void *item) {
    if (q == NULL|| item == NULL) {errno = EINVAL; return -1;}
    //eseguo una lock sulla mutex per la sezione critica del codice
    mtx_lock(&q->lock);
    /* while, non if: al risveglio la condizione va sempre riverificata
      (spurious wakeup, o un altro producer potrebbe aver già
       rioccupato lo spazio liberato prima che questo thread riprenda). */
    while (q->count == q->capacity) {
        cnd_wait(&q->not_full, &q->lock);
    }
    //aggiungo l'item in coda
    q->items[q->tail] = item;
    //aggiorno il count
    q->count++;
    //aggiorno la coda(natura circloare)
    q->tail = (q->tail + 1) % q->capacity;

    /* Sveglia un eventuale consumer in attesa: ora c'è almeno un elemento disponibile. */
    cnd_signal(&q->not_empty);
    mtx_unlock(&q->lock);
    return 0;
}

int mr_queue_pop(mr_queue_t *q, void **item) {
    if (q == NULL|| item == NULL) {errno = EINVAL; return -1;}
    //eseguo una lock sulla mutex per la sezione critica del codice
    mtx_lock(&q->lock);
    /* Aspetta finché non c'è qualcosa da consumare, a meno che la coda non sia stata chiusa:
     in tal caso esce comunque dal loop,per gestire il caso "chiusa e vuota" subito sotto. */
    while (q->count == 0 && !q->closed) {
        cnd_wait(&q->not_empty, &q->lock);
    }

    /* Questo controllo va fatto SOLO dopo il while, mai prima: se un
     * thread controllasse q->closed subito, senza aver aspettato,
     * potrebbe leggere closed == 1 mentre la coda contiene ancora
     * elementi non consumati, e quegli elementi andrebbero persi
     * silenziosamente. closed significa "non arriveranno più NUOVI
     * elementi", non "puoi ignorare quelli già presenti". */
    if (q->count == 0 && q->closed) {
        mtx_unlock(&q->lock);
        return 1; /* fine del flusso: nessun elemento da restituire */
    }
    // assegna l'elemento uscente dalla coda a item
    *item = q->items[q->head];
    //aggiorna count
    q->count--;
    //aggiorna la testa della coda(natura circolare)
    q->head = (q->head + 1) % q->capacity;

    /* Sveglia un eventuale producer in attesa: ora c'è spazio libero. */
    cnd_signal(&q->not_full);
    mtx_unlock(&q->lock);
    return 0;
}

int mr_queue_close(mr_queue_t *q) {
    if (q == NULL) {errno = EINVAL; return -1;}
    mtx_lock(&q->lock);
    q->closed = 1;
    /* broadcast, non signal: possono esserci più consumer in attesa
       su not_empty, e devono TUTTI essere svegliati per poter
       ricontrollare la condizione di chiusura e uscire dal loop
       (un solo signal ne sveglierebbe uno solo, lasciando gli altri
       bloccati indefinitamente). */
    cnd_broadcast(&q->not_empty);
    mtx_unlock(&q->lock);
    return 0;
}