#ifndef MR_H
#define MR_H

#include <stddef.h>

/* Handle opaco di una elaborazione. */
typedef struct mr *mr_t;

/*
 * [ADDENDUM] Funzione di hashing opzionale per partizionare i token
 * fra i thread reducer interni al processo reducer.
 * Deve essere deterministica: due occorrenze dello stesso token
 * devono produrre lo stesso valore di hash. Non deve modificare il
 * token né dipendere dall'ordine di esecuzione dei thread.
 */
typedef size_t (*mr_hash_t)(const char *token,size_t token_len,void *user_arg);

//struct per gli attr di mr
typedef struct {
    /* numero di thread worker usati nel processo mapper (>= 1) */
    size_t mapper_threads;
    /* numero di thread worker usati nel processo reducer (>= 1) */
    size_t reducer_threads;

    /* capacità massima (in numero di elementi) delle code produttore-consumatore interne 
       usate da mapper*/
    size_t queue_size;
    /* percorso del file di log, NULL = usa il nome di default */
    const char *log_file;

    /* [ADDENDUM] hash custom per l'assegnazione token -> thread reducer.
     * Se hash == NULL, il framework usa una funzione di default.(dbj2) */
    mr_hash_t hash;
    void *hash_arg;

    /* [ADDENDUM] percorso file statistiche finali, NULL = nessun file. */
    const char *stats_file;
} mr_attr_t;

//struct per rappresentare una linea passata da main->mapper
typedef struct {
    /* nome del file da cui proviene la riga; puntatore valido solo
       durante l'invocazione della funzione mapper, non necessariamente
       terminato da '\0' (usare file_name_len per la lunghezza reale) */
    const char *file_name;
    size_t file_name_len;

    /* numero della riga all'interno del file, parte da 1 */
    unsigned long line_number;

    /* contenuto della riga, senza il carattere di fine riga; puntatore
       valido solo durante l'invocazione della funzione mapper, non
       necessariamente terminato da '\0' (usare line_len) */
    const char *line;
    size_t line_len;

} mr_file_line_t;

// struct per rappresentare un <processed token> dopo che il mapper ha elaborato mr_file_line_t
typedef struct {
    /* dato opaco: il framework non lo interpreta mai (non assume che
       sia un intero, una stringa C, terminato da '\0', ecc.).
       Può essere NULL se size vale 0 */
    const void *data;
    size_t size;

} mr_value_t;

/*Funzione (fornita dal framework al mapper) usata per emettere una coppia <token,value>.
  token deve essere una stringa C valida, terminata da '\0', composta solo da caratteri
  alfanumerici ASCII; value è una sequenza opaca di value_size byte (può essere NULL se
  value_size è 0). Il framework copia il contenuto prima che la chiamata ritorni, quindi
  l'utente può riusare o liberare i buffer subito dopo,
  Ritorna 0 se la coppia è stata emessa correttamente
  Ritorna -1 se c'è un problema*/
typedef int (*mr_emit_pair_t)(const char *token,const void *value,size_t value_size,void *emit_arg);

/*Funzione (fornita dal framework al reducer) usata per emettere un risultato finale.
  token deve essere una stringa C valida (nel progetto base coincide con il token
  ricevuto dalla funzione reducer); result è una sequenza opaca di result_size byte
  (può essere NULL se result_size è 0). Il framework copia il contenuto prima che la
  chiamata ritorni, quindi l'utente può riusare o liberare i buffer subito dopo,
  Ritorna 0 se il risultato è stato emesso correttamente
  Ritorna -1 se c'è un problema*/
typedef int (*mr_emit_result_t)(const char *token,const void *result,size_t result_size,void *emit_arg);

/*Funzione mapper fornita dal programma utente (non è una funzione thread, è invocata
  dai thread worker del processo mapper): riceve una riga logica (line, valida solo
  durante l'invocazione) e può emettere zero o più coppie <token,value> chiamando emit.
  user_arg è quello passato a mr_create, propagato senza interpretazione,
  Ritorna 0 in caso di successo
  Ritorna -1 se c'è un problema (interrompe l'elaborazione)*/
typedef int (*mr_mapper_t)(const mr_file_line_t *line,mr_emit_pair_t emit,void *emit_arg,void *user_arg);

/*Funzione reducer fornita dal programma utente (non è una funzione thread, è invocata
  dai thread worker del processo reducer, una sola volta per token distinto): riceve
  il token e tutti i values associati (values validi solo durante l'invocazione), e può
  emettere zero o più risultati chiamando emit. user_arg è quello passato a mr_create,
  propagato senza interpretazione,
  Ritorna 0 in caso di successo
  Ritorna -1 se c'è un problema (interrompe l'elaborazione)*/
typedef int (*mr_reducer_t)(const char *token,const mr_value_t *values,size_t values_count,mr_emit_result_t emit,void *emit_arg,void *user_arg);

/*Funzione per inizializzare attr(definito all'interno di mr.h),
  Ritorna 0 dopo aver settato tutti i campi di attr
  Ritorna -1 se c'è un problema*/
int mr_attr_init(mr_attr_t *attr);

/*Funzione per liberare attr(definito all'interno di mr.h),
  Ritorna 0 dopo aver liberato i campi con il file di log e stats(copie locali ma definite const)
  Ritorna -1 se c'è un problema*/
int mr_attr_destroy(mr_attr_t *attr);

/*Funzione per settare il numero di worker-mapper in attr(definito all'interno di mr.h,attr->mapper_thread) fa controllo > 0 su n,
  Ritorna 0 dopo aver settato il numero di worker mapper
  Ritorna -1 se c'è un problema*/
int mr_attr_set_mapper_threads(mr_attr_t *attr, size_t n);

/*Funzione per settare il numero di worker-reducer in attr(definito all'interno di mr.h,attr->reducerr_thread) fa controllo > 0 su n,
  Ritorna 0 dopo aver settato il numero di worker reducer
  Ritorna -1 se c'è un problema*/
int mr_attr_set_reducer_threads(mr_attr_t *attr, size_t n);

/*Funzione per settare la queue.capacity in attr(definito all'interno di mr.h,attr->queue_size) fa controllo > 0 su n,
  Ritorna 0 dopo aver settato il numero di worker mapper
  Ritorna -1 se c'è un problema*/
int mr_attr_set_queue_size(mr_attr_t *attr, size_t n);

/*Funzione per settare il path del log in attr(definito all'interno di mr.h,attr->log_file) fa una copia locale, se passato NULL viene indicato il default,
  Ritorna 0 dopo aver copiato il contenuto di path in un buffer e aver settato il puntatore attr->log_file a quel buffer
  Ritorna -1 se c'è un problema*/
int mr_attr_set_log_file(mr_attr_t *attr, const char *path);

/*Funzione per settare il path delle stats in attr(definito all'interno di mr.h,attr->stats_file) fa una copia locale, se viene passato NULL allora indica che il file stats non è richiesto,
  Ritorna 0 dopo aver copiato il contenuto di path in un buffer e aver settato il puntatore attr->stats_file a quel buffer
  Ritorna -1 se c'è un problema*/
int mr_attr_set_stats_file(mr_attr_t *attr, const char *path);

/*Funzione per settare la funzione di hashingin attr(definito all'interno di mr.h,attr->hash) e il suo argomento(attr->hash_arg),se passato HASH = NULL viene indicato il default
  Ritorna 0 dopo aver settato la funzione di hash ed il suo argomento
  Ritorna -1 se c'è un problema*/
int mr_attr_set_hash_function(mr_attr_t *attr, mr_hash_t hash, void *hash_arg);


/*Funzione per creare una nuova elaborazione: alloca la struct mr opaca, ne copia
  internamente gli attributi (deep copy di attr, così l'utente può distruggere o
  modificare la propria mr_attr_t dopo la chiamata senza alterare l'elaborazione),
  salva i puntatori a mapper/reducer forniti dall'utente e user_arg (passato senza
  interpretazione), e fa puntare *mr alla struct appena allocata

  Ritorna 0 dopo aver completato l'allocazione e la copia con successo
  Ritorna -1 se c'è un problema */
int mr_create(mr_t *mr,const mr_attr_t *attr,mr_mapper_t mapper,mr_reducer_t reducer,void *user_arg);

/*Funzione bloccante che avvia l'elaborazione vera e propria: crea le pipe, i processi
  mapper e reducer (fork+dup2), invia le righe di input_path (file singolo o directory
  scandita ricorsivamente) al mapper, raccoglie i risultati dal reducer e li scrive in
  output_path nel formato a record descritto in relazione. Ritorna il controllo al
  chiamante solo quando l'elaborazione è terminata o si è verificato un errore,
  Ritorna 0 dopo aver prodotto correttamente il file di output
  Ritorna -1 se c'è un problema*/
int mr_start(mr_t mr, const char *input_path, const char *output_path);

/*Funzione per rilasciare le risorse associate a un'elaborazione (definito in mr.h,
  la struct mr puntata da mr_t): libera attr (mr_attr_destroy), chiude il file di log
  e rimuove il semaforo POSIX associato, poi libera la struct mr stessa,

  Ritorna 0 dopo aver liberato tutte le risorse
  Ritorna -1 se c'è un problema (mr NULL)*/
int mr_destroy(mr_t mr);

#endif
