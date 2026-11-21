#include "mr_internal.h"
#include "mr_log.h"

#include "utilities/mr_protocol_utilities.h"
#include "utilities/mr_queue_utilities.h"
#include "utilities/mr_token_value_list_utilities.h"
#include "utilities/mr_token_result_list_utilities.h"
#include "utilities/mr_hash_utilities.h"

#include <unistd.h>
#include <threads.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

//struct per il context di reducer da passare come void e fare re-cast
typedef struct {
    //lista dei gruppi <token, value[]> letti da stdin popolata direttamente dal reader
    mr_tokens_and_values_list_t pair_list;
    //context per il logging
    mr_log_ctx_t* log_ctx;
    //numero di worker/bucket (== mr->attr.reducer_threads)
    size_t num_workers;
    //array di N liste (bucket), una per worker, popolate dallo smistamento hash-based
    mr_tokens_and_values_list_t* workers_pair_list;
    //lista dei risultati prodotti dai worker, accumulati qui prima di essere ordinati e scritti
    mr_token_result_pair_list_t result_list;
    //fd di STDOUT [la pipe verso il processo principale, grazie a dup2]
    int fd_out;
    //funzione di reducing passata dallo user
    mr_reducer_t user_reducer;
    //lock per la scrittura di emit (accumulo in result_list)
    mtx_t lock;
    //campo da passare al reducer come arg aggiuntivo
    void *user_arg;

    /* [ADDENDUM] funzione di hash (custom o mr_default_hash) usata per
       decidere a quale thread/coda reducer instradare ciascun token.
       Idea: reducer_threads code separate, una per worker; il token va
       nella coda numero hash(token, len, hash_arg) % reducer_threads.
       Così token uguali finiscono sempre allo stesso thread, senza bisogno
       di sincronizzare l'intera struttura di raggruppamento fra i worker. */
    mr_hash_t hash;
    void *hash_arg;
} reducer_ctx_t;

//struct passata a ogni thread worker: il proprio indice di bucket + il context condiviso
typedef struct{
    //indice del bucket (workers_pair_list[worker_id]) di competenza di questo worker
    size_t worker_id;
    //puntatore al context condiviso, comune a tutti i worker
    reducer_ctx_t* reducer_ctx;
}worker_reducer_ctx_t;

/* Thread che legge le coppie da stdin e le raggruppa direttamente per token
   (nessuna coda intermedia: il raggruppamento richiede comunque di vedere
   l'intero flusso prima di poter considerare un gruppo completo) */
static int reader_main(void *arg) {
    //faccio il cast di arg a reducer_ctx_t* per rispettare la firma necessaria per thrd_create
    reducer_ctx_t* reducer_ctx = (reducer_ctx_t*) arg;

    //dichiaro le variabili che riempirò con recv_pair_msg{header e payload}
    mr_token_value_pair_t temp;
    void* value_data;

    //inf mi dice lo stato della recv
    int inf;

    //ricevo finché inf è 0
    while((inf = recv_pair_msg(STDIN_FILENO,&(temp.token),&(value_data),&(temp.value.size),&(temp.token_len))) == 0)
    {
        //impacchetto la coppia ricevuta
        mr_token_value_pair_t token_value_pair;
        token_value_pair.token = temp.token;
        token_value_pair.token_len = temp.token_len;
        token_value_pair.value.data = value_data;
        token_value_pair.value.size = temp.value.size;
        //la aggiungo al gruppo giusto (o ne creo uno nuovo), raggruppando per token
        if(mr_tokens_and_values_add(&(reducer_ctx->pair_list),token_value_pair)){return -1;}
    }
    //se inf == 1 [EOF STDIN] allora ho letto tutte le coppie, il raggruppamento in pair_list è completo
    if (inf == 1)
    {
        return 0;
    }
    //se inf == -1 allora c'è stato un problema
    else{
        return -1;
    }
}

/*Callback passata come mr_emit_result_t alla funzione reducer utente.
  Copia <token, result> e li accumula in result_list (non scrive subito su
  fd_out: la scrittura ordinata avviene solo dopo che tutti i worker hanno
  finito, per garantire il determinismo dell'output), sincronizzando
  l'accesso fra i thread reducer con un mutex dedicato in reducer_ctx.*/
static int emit_result_callback(const char *token, const void *result,size_t result_size, void *emit_arg) {
    //faccio il cast di emit_arg a reducer_ctx_t* per rispettare la firma
    reducer_ctx_t* reducer_ctx = (reducer_ctx_t*) emit_arg;

    //copio il token in un buffer posseduto dal framework
    mr_token_result_pair_t temp;
    temp.token = (char*)malloc(strlen(token)+1);
    if(temp.token == NULL){return -1;}
    strcpy(temp.token,token);

    //copio il result (dato opaco, niente terminatore) in un buffer posseduto dal framework
    temp.result_len = result_size;
    temp.result= malloc(temp.result_len);
    if(temp.result == NULL){free(temp.token);return -1;}
    memcpy(temp.result, result, result_size);

    //sezione critica del codice: più worker possono chiamare emit contemporaneamente
    mtx_lock(&reducer_ctx->lock);
    if(mr_token_result_pair_list_add_realloc(&(reducer_ctx->result_list),temp) == -1){
        //in caso di errore libero comunque la lock
        mtx_unlock(&(reducer_ctx->lock));
        return -1;
    }
    //libero comunque la lock
    mtx_unlock(&(reducer_ctx->lock));
    return 0;
}

/*Thread worker: prende i gruppi completi <token, values[]> del proprio bucket
  e invoca la funzione reducer dell'utente una sola volta per gruppo, che a
  sua volta chiama emit() per produrre zero o più risultati.*/
static int reducer_worker_main(void *arg) {
    //faccio il cast di arg a worker_reducer_ctx_t* per rispettare la firma necessaria per thrd_create
    worker_reducer_ctx_t* worker_reducer_ctx = (worker_reducer_ctx_t*) arg;
    reducer_ctx_t* reducer_ctx = worker_reducer_ctx->reducer_ctx;
    //il bucket di competenza di questo worker
    size_t my_bucket = worker_reducer_ctx->worker_id;
    mr_tokens_and_values_list_t* tksvs = &(reducer_ctx->workers_pair_list[my_bucket]);

    //scorro tutti i gruppi del mio bucket
    for (size_t i = 0; i < tksvs->count; i++)
    {
        mr_token_values_list_t* tkvs = &(tksvs->arr[i]);
        //chiamo la funzione reducer dello user sul gruppo completo (token + tutti i value)
        if(reducer_ctx->user_reducer((tkvs->token),tkvs->arr,(tkvs->count),emit_result_callback,reducer_ctx,reducer_ctx->user_arg)==-1){
            return -1;
        }
    }

    return 0;
}

/*Coordina il processo reducer: crea il thread reader (che raggruppa le
  coppie per token), smista ogni gruppo completo in uno dei reducer_threads
  bucket via hash(token) % reducer_threads, crea i thread worker che
  elaborano ciascuno il proprio bucket, e infine ordina e scrive in
  sequenza tutti i risultati accumulati.*/
void reducer_process_main(struct mr *mr) {
    reducer_ctx_t reducer_ctx;
    int inf;

    //VLA per i bucket: un array di N liste, N = reducer_threads
    mr_tokens_and_values_list_t buckets[mr->attr.reducer_threads];

    //inizializzo ogni bucket (sono N liste distinte, ognuna va inizializzata)
    for (size_t i = 0; i < mr->attr.reducer_threads; i++) {
        mr_tokens_and_values_init(&buckets[i]);
    }

    //popolo i campi del context usando mr ereditato dal padre che ha chiamato il fork
    reducer_ctx.workers_pair_list = buckets;
    reducer_ctx.log_ctx = &(mr->mr_log_ctx);
    reducer_ctx.num_workers = mr->attr.reducer_threads;
    reducer_ctx.fd_out = STDOUT_FILENO;
    reducer_ctx.user_reducer = mr->reducer;
    reducer_ctx.user_arg = mr->user_arg;

    //se l'utente non ha specificato una hash custom, uso quella di default
    reducer_ctx.hash = mr->attr.hash ? mr->attr.hash : mr_default_hash;
    reducer_ctx.hash_arg = mr->attr.hash_arg;
    mtx_init(&reducer_ctx.lock, mtx_plain);   //serve  per emit_result_callback

    //inizializzo pair_list (dove il reader accumula) e result_list (dove i worker accumulano)
    mr_tokens_and_values_init(&reducer_ctx.pair_list);
    mr_token_result_pair_list_init(&reducer_ctx.result_list);

    //creo il thread reader e aspetto che finisca (deve completare il raggruppamento prima dello smistamento)
    thrd_t reader;
    if(thrd_create(&reader,reader_main,&reducer_ctx) != thrd_success){_exit(1);}

    //[LOG] faccio logging dello start del thread reader
    mr_log_write(reducer_ctx.log_ctx, "reducer", "THREAD_START", "reader", "thread reader avviato");
    thrd_join(reader, &inf);
    if (inf != 0) { _exit(1); }
    //[LOG] faccio logging della fine del thread reader
    mr_log_write(reducer_ctx.log_ctx, "reducer", "THREAD_END", "reader", "thread reader terminato");
    //[LOG] uso pair_list.count per loggare il numero di token distinti raggruppati, usato anche per le stats
    mr_log_write(reducer_ctx.log_ctx, "reducer", "DISTINCT_TOKENS", "-", "token distinti raggruppati: %zu", reducer_ctx.pair_list.count);

    //SMISTAMENTO: per ogni gruppo completo in pair_list, calcolo l'hash e lo inserisco nel bucket giusto
    for (size_t i = 0; i < reducer_ctx.pair_list.count; i++) {
        mr_token_values_list_t group = reducer_ctx.pair_list.arr[i];
        size_t bucket_idx = reducer_ctx.hash(group.token, group.token_len, reducer_ctx.hash_arg) % mr->attr.reducer_threads;
        mr_tokens_and_values_add_realloc(&buckets[bucket_idx],&group);
    }

    //dichiaro un array di context, uno per ogni worker (ciascuno col proprio worker_id/bucket)
    worker_reducer_ctx_t worker_reducer_ctx[mr->attr.reducer_threads];
    //dichiaro un array di thread ids, uno per ogni worker
    thrd_t reducer_workers[mr->attr.reducer_threads];
    for (size_t i = 0; i < mr->attr.reducer_threads; i++)
    {
        //popolo il context specifico di questo worker
        worker_reducer_ctx[i].worker_id = i;
        worker_reducer_ctx[i].reducer_ctx = &reducer_ctx;
        if(thrd_create(&reducer_workers[i],reducer_worker_main,&worker_reducer_ctx[i]) != thrd_success){ _exit(1);}
        //[LOG] faccio logging dello start del thread worker #i
        mr_log_write(reducer_ctx.log_ctx, "reducer", "THREAD_START", "worker", "thread worker %zu avviato", i);
    }

    //faccio la join su tutti i worker ed aspetto che abbiano finito
    for (size_t i = 0; i < mr->attr.reducer_threads; i++)
    {
        thrd_join(reducer_workers[i],&inf);
        if(inf != 0){
            _exit(1);
        }
    }
    //[LOG] faccio logging della fine di tutti i worker thread
    mr_log_write(reducer_ctx.log_ctx, "reducer", "THREAD_END", "-", "tutti i thread worker terminati");

    //ordino i risultati accumulati lessicograficamente per token, PRIMA di scriverli
    mr_token_result_pair_list_reorder(&reducer_ctx.result_list);

    //scrivo i risultati in sequenza 
    for (size_t i = 0; i < reducer_ctx.result_list.count; i++) {
        mr_token_result_pair_t* r = &reducer_ctx.result_list.arr[i];
        if (send_result_msg(reducer_ctx.fd_out, r->token, r->result, r->result_len) == -1) {
            _exit(1);
        }
        //libero token/result subito dopo averli scritti
        free(reducer_ctx.result_list.arr[i].token);
        free(reducer_ctx.result_list.arr[i].result);
    }

    //distruggo la mutex per la emit
    mtx_destroy(&reducer_ctx.lock);

    //libero ogni bucket (contenuto + array): qui vive davvero la memoria dei gruppi
    for (size_t i = 0; i < mr->attr.reducer_threads; i++) {
        mr_tokens_and_values_destroy(&buckets[i]);
    }

    //libero solo il contenitore di pair_list: il contenuto è già stato trasferito e liberato via buckets[i]
    free(reducer_ctx.pair_list.arr);

    //[LOG] uso result_list.count per loggare il numero di risultati finali prodotti, usato anche per le stats
    mr_log_write(reducer_ctx.log_ctx, "reducer", "RESULTS_EMITTED", "-", "risultati finali prodotti: %zu", reducer_ctx.result_list.count);

    mr_token_result_pair_list_destroy(&reducer_ctx.result_list);

    //chiudo STDOUT segnalando EOF al processo principale, solo dopo che tutto è stato scritto
    close(reducer_ctx.fd_out);
}