#include "mr_internal.h"
#include "mr_log.h"

#include "utilities/mr_protocol_utilities.h"
#include "utilities/mr_queue_utilities.h"

#include <unistd.h>
#include <threads.h>
#include <stdlib.h>

//struct per il context di mapper da passare come void e  fare re-cast
typedef struct {
    // queue circolare che viene popoplata dal reader e consumata dai worker
    mr_queue_t line_queue;
    //funzione di mapping passata dallo user
    mr_mapper_t user_mapper;
    // campo da passare al mapper come arg aggiuntivo
    void *user_arg;
    //context per il logging 
    mr_log_ctx_t *log_ctx;   
    //counter per logging/stats
    size_t pairs_produced;
    //lock per la scrittura di emit
    mtx_t lock;
    //fd di STDOUT[la pipe a reducer grazie a dup2]
    int fd_out;
} mapper_ctx_t;

/* Thread che legge righe da stdin e le inserisce nella coda line_queue */
static int reader_main(void *arg) {
    //faccio il cast di arg a mapper_ctx_t* per rispettare la firma necessaria per thrd_create
    mapper_ctx_t* mapper_ctx = (mapper_ctx_t*) arg;

    // dichiaro le variabili che riempirò con recv_line_msg{header e payload}
    mr_line_header_t temp;
    char* temp_line;
    char* temp_file_name;

    // inf mi dice lo stato della recv
    int inf;

    //ricevo finche inf è 0
    while((inf = recv_line_msg(STDIN_FILENO, &(temp_file_name), &(temp.file_name_len),&(temp.line_number),&(temp_line),&(temp.line_length))) == 0)
    {
        //alloco lo spazio per mr_file_line_t che rappresenta la line con tutte le informazioni necessarie per i workers
        mr_file_line_t* mapper_line = (mr_file_line_t*)malloc(sizeof(mr_file_line_t));
        if(mapper_line == NULL){ free(temp_file_name);free(temp_line);return -1;}
        //popolo mapper_line
        mapper_line->file_name = temp_file_name;
        mapper_line->file_name_len = temp.file_name_len;
        mapper_line->line_number = temp.line_number;
        mapper_line->line = temp_line;
        mapper_line->line_len = temp.line_length;
        mr_queue_push(&(mapper_ctx->line_queue),mapper_line);
    }
    //se inf = 1 [EOF STDIN] allora ho letto tutto ciò che c'era nella pipe e quindi segnalo alla coda che non arriverà altro
    if (inf == 1)
    {
        //setto mapper_ctx->line_queue.closed = 1 per dire che non ARRIVERANNO altri elementi per i consumer
        mr_queue_close(&(mapper_ctx->line_queue));
        return 0;
    }
    //se inf == -1 allora c'è stato un problema
    else{
        //setto mapper_ctx->line_queue.closed = 1 per dire che non ARRIVERANNO altri elementi per i consumer
         mr_queue_close(&(mapper_ctx->line_queue));
        return -1;
    }
}


/*Callback passata come mr_emit_pair_t alla funzione mapper utente.
  Deve serializzare <token, value> e scriverli su fd_out, sincronizzando
  l'accesso fra i vari thread mapper (mutex dedicato in mapper_ctx ).*/
static int emit_pair_callback(const char *token, const void *value,size_t value_size, void *emit_arg) {
    //faccio il cast di emit_arg a mapper_ctx_t* per rispettare la firma n
    mapper_ctx_t* mapper_ctx = (mapper_ctx_t*) emit_arg;
    //sezione critica del codice,
    mtx_lock(&(mapper_ctx->lock));
    // scrivo su fd_out il risultato dello user mapper
    if(send_pair_msg(mapper_ctx->fd_out,token,value,value_size) == -1){
        //in caso di errore libero comunque la lock
        mtx_unlock(&(mapper_ctx->lock));
        return -1;
    }
    //aggiorno pairs_produced per usarlo nel logging e nelle statistiche
    mapper_ctx->pairs_produced++;
    // libero comunque la lock
    mtx_unlock(&(mapper_ctx->lock));
    return 0;
}


/*Thread worker: estrae righe dalla coda, invoca la funzione mapper
  dell'utente, che a sua volta chiama emit() per produrre coppie.
  emit() deve serializzare e scrivere la coppia su fd_out(sincronizzandosi con gli altri thread worker).
  free(line);
 */
static int mapper_worker_main(void *arg) {
    //faccio il cast di arg a mapper_ctx_t* per rispettare la firma necessaria per thrd_create
    mapper_ctx_t* mapper_ctx = (mapper_ctx_t*) arg;
    //dichiaro il guscio che conterrà mapper_line
    void* temp;
    while(!mr_queue_pop(&(mapper_ctx->line_queue),&temp)){
        //casto il contenuto di line_queue a mr_file_line_t
        mr_file_line_t* mapper_line = (mr_file_line_t*)temp;
        //chiamo la funzione mapper dello user su mapper_line
        if(mapper_ctx->user_mapper(mapper_line,emit_pair_callback,mapper_ctx,mapper_ctx->user_arg) == -1){ return -1;};
        //livero il contenuto degli elementi di line_queue 
        free((void*)mapper_line->file_name);
        free((void*)mapper_line->line);
        free(mapper_line);
    }
    return 0;
}

    /* 
     * - inizializza mapper_ctx_t (coda, fd_out, puntatore a mr->mapper, mr->user_arg)
     * - crea 1 thread reader_main (legge da fd_in / stdin)
     * - crea mr->attr.mapper_threads thread mapper_worker_main
     * - thrd_join su tutti
     * - SOLO DOPO che tutti i worker sono terminati: close(fd_out)
     *   per segnalare EOF al reducer
     */
void mapper_process_main(struct mr *mr) {

    //dichiaro la struct  mapper_ctx_t mapper_ctx;
    mapper_ctx_t mapper_ctx;
    //ne popolo i campi usando mr ereditato dal padre che ha chiamato il fork
    mapper_ctx.user_arg = mr->user_arg;
    mtx_init(&mapper_ctx.lock,mtx_plain);
    mapper_ctx.user_mapper = mr->mapper;
    mapper_ctx.fd_out = STDOUT_FILENO;
    mapper_ctx.log_ctx = &(mr->mr_log_ctx);
    mapper_ctx.pairs_produced = 0;
    // inizializzo la coda che verrà usata dal reader e dai workers
    if(mr_queue_init(&(mapper_ctx.line_queue),mr->attr.queue_size) == -1){_exit(1);}
    


    //creo il thread reader che inizierà a popolare la coda e gli passo &mapper_ctx come arg
    thrd_t reader;
    if(thrd_create(&reader,reader_main,&mapper_ctx) != thrd_success){_exit(1);};

    //[LOG] faccio logging dello start del thread reader
    mr_log_write(mapper_ctx.log_ctx, "mapper", "THREAD_START", "reader", "thread reader avviato");

    // dichiaro un array di thread ids, uno per ogni worker
    thrd_t mapper_workers[mr->attr.mapper_threads];

    for (size_t i = 0; i < mr->attr.mapper_threads; i++)
    {   //popolo l'array di workers e gli passo &mapper_ctx come arg
        if(thrd_create(&mapper_workers[i],mapper_worker_main,&mapper_ctx) != thrd_success){ _exit(1);}
        //[LOG] faccio logging dello start del thread worker #i
        mr_log_write(mapper_ctx.log_ctx, "mapper", "THREAD_START", "worker", "thread worker %zu avviato", i);
    }

    //inf mi dice lo status delle thrd_join
    int inf; 
    //faccio la join sul reader ed aspetto che abbia finito
    thrd_join(reader,&inf);
    if(inf != 0){_exit(1);}

    //[LOG] faccio logging della fine del thread reader
    mr_log_write(mapper_ctx.log_ctx, "mapper", "THREAD_END", "reader", "thread reader terminato");

    //faccio la join sui workers ed aspetto che abbiano finito con i rimasugli nella coda
    for (size_t i = 0; i < mr->attr.mapper_threads; i++)
    {
        thrd_join(mapper_workers[i],&inf);
        if(inf != 0){_exit(1);}
    }
    //[LOG] faccio logging della fine di tutti i worker threads
    mr_log_write(mapper_ctx.log_ctx, "mapper", "THREAD_END", "-", "tutti i thread worker terminati");

    //distruggo la mutex per la emit
    mtx_destroy(&(mapper_ctx.lock));

    //una volta che la coda è totalmente vuota dopo la fine di tutti i thread workers allora ne elimino il guscio
    if(mr_queue_destroy(&(mapper_ctx.line_queue))== -1){_exit(1);};

    //[LOG] uso la variabile mapper_ctx.pairs_produced per loggare tutti i pairs produced dallo user_mapper, questo verrà poi usato anche per le stats
    mr_log_write(mapper_ctx.log_ctx, "mapper", "PAIRS_PRODUCED", "-", "coppie prodotte: %zu", mapper_ctx.pairs_produced);

    //chiudo STDOUT segnalando EOF al Reducer
    if (close(mapper_ctx.fd_out)== -1){_exit(1);};
}
