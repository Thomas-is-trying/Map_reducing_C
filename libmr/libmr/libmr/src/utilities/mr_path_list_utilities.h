#ifndef MR_PATH_LIST_UTILITIES_H
#define MR_PATH_LIST_UTILITIES_H

#include "../mr_log.h"

#include <sys/types.h>

//capacity iniziale per path_list
#define MR_PATH_LIST_START_CAPACITY 1
//fattore di crescita per la realloc
#define MR_PATH_LIST_GROW_FACTOR 2

/*struct per contenere la lista di tutti  i path ai file;*/
typedef struct {
    //inizializzata a PATH_LIST_START_CAPACITY e cresce con mr_path_list_add_realloc con fattore PATH_LIST_GROW_FACTOR
    size_t capacity;
    size_t count;
    //contatore per stats finali
    size_t total_line_count;
    //puntatore ad un puntatore di arr
    char** arr;
} mr_path_list_t;



/*Funzione per inizializzare mr_path_list_t, con PATH_LIST_START_CAPACITY
  Ritorna 0 dopo aver settato tutti i campi di mr_path_list_t,
  Ritorna -1 se c'è un problema*/
int mr_path_list_init(mr_path_list_t* l);

/*Funzione per aggiungere un path p a mr_path_list_t* l, utilizza realloc(con PATH_LIST_GROW_FACTOR) e riassegna il puntatore a l->arr
  Ritorna 0 dopo aver aggiunto p a l,
  Ritorna -1 se c'è un problema*/
int mr_path_list_add_realloc(mr_path_list_t* l, char* p);

/*Funzione per riordinare mr_path_list_t* l, utilizza qsort(con cmp a strcmp), utilizzata per mantenere il determinismo durante l'esecuzione
  Ritorna 0 dopo aver riordinato l
  Ritorna -1 se c'è un problema*/
int mr_path_list_reorder(mr_path_list_t* l);

/*Funzione per liberare mr_path_list_t* l, libera tutti gli elementi e poi ne libera il guscio
  Ritorna 0 dopo aver liberato l
  Ritorna -1 se c'è un problema*/
int mr_path_list_destroy(mr_path_list_t* l);

/*Funzione ricorsiva per scansionare le dir e le sub-dir,
  scansiona dir e chiama se stessa su ogni elemento
  Ritorna 0 dopo aver finito la scansione
  Ritorna -1 se c'è un problema*/
int mr_path_scan_recursive(char* path,mr_path_list_t* l);

/*Funzione ausiliaria per creare il path assoluto di un elemento dal suo nome e dal path usato
 per arrivare alla dir in cui si trova, mette il risultato in result
  Ritorna 0 dopo aver calcolato il path assoluto e averlo assegnato
  Ritorna -1 se c'è un problema*/
int mr_path_build_from_name(char* filename, char* path,char** result);

/*
  Funzione che usa un puntatore alla funzione sender per mandare 
  il contenuto di tutti i file di l diviso in righe su fd(prende  mr_log_ctx_t *log_ctx per il logging di apertura e chiusura input)
  Ritorna 0 dopo aver mandato tutti i line di tutti i file all'interno di  l 
  Ritorna -1 se c'è un problema*/
int mr_path_list_send_lines_to_mapper(mr_path_list_t* l,int (*sender)(int fd, const char *file_name, size_t file_name_len,unsigned long line_number, const char *line, size_t line_len),int fd, mr_log_ctx_t *log_ctx);
/*
  Funzione che usa un puntatore alla funzione receiver per ricevere da fd 
  coppie token/result e scriverle sul file indicato da output_path (prende  mr_log_ctx_t *log_ctx per il logging di apertura e chiusura output)
  Ritorna 0 dopo aver ricevuto e scritto tutti i dati
  Ritorna -1 se c'è un problema*/
int mr_receive_from_reducer(int (*receiver)(int fd, char **token, void **result, size_t *result_len,size_t *token_len),int fd,char* output_path,mr_log_ctx_t *log_ctx);


#endif