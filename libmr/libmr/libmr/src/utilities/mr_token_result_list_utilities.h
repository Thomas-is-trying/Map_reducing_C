#ifndef MR_TOKEN_RESULT_LIST_UTILITIES_H
#define MR_TOKEN_RESULT_LIST_UTILITIES_H
#include <sys/types.h>

//capacity iniziale per mr_token_values_list
#define MR_TOKEN_RESULT_LIST_START_CAPACITY 1
//fattore di crescita per la realloc
#define MR_TOKEN_RESULT_LIST_GROW_FACTOR 2

/*struct per contenere un pair di [token,result]*/
typedef struct{
    char* token;
    //result è un dato opaco quindi serve conoscerne la lunghezza
    size_t result_len;
    void* result;
}mr_token_result_pair_t;

/*struct per contenere un una lista di pair[token,result]*/
typedef struct{
    size_t capacity;
    size_t count;
    mr_token_result_pair_t* arr;
}mr_token_result_pair_list_t;

/*Funzione per inizializzare mr_token_result_pair_list_t con MR_TOKEN_RESULT_LIST_START_CAPACITY
  Ritorna 0 dopo aver settato tutti i campi di mr_token_result_pair_list_t,
  Ritorna -1 se c'è un problema*/
int mr_token_result_pair_list_init(mr_token_result_pair_list_t* l);

/*Funzione per aggiungere un mr_token_result_pair_t tr a mr_token_result_pair_list_t* l, 
  utilizza realloc(con MR_TOKEN_RESULT_LIST_GROW_FACTOR) e riassegna il puntatore a l->arr
  Ritorna 0 dopo aver aggiunto tr a l,
  Ritorna -1 se c'è un problema*/
int mr_token_result_pair_list_add_realloc(mr_token_result_pair_list_t* l, mr_token_result_pair_t tr);

 /*Funzione per riordinare mr_token_result_pair_list_t* l, utilizza qsort(con cmp a strcmp),
  utilizzata per mantenere il determinismo durante l'esecuzione
  Ritorna 0 dopo aver riordinato l
  Ritorna -1 se c'è un problema*/
int mr_token_result_pair_list_reorder(mr_token_result_pair_list_t* l);

/*Funzione per liberare mr_token_result_pair_list_t* l, libera il guscio dopo che gli
  elementi sono già stati liberati dal reducer_process_main  dopo averli mandati al main
  Ritorna 0 dopo aver liberato l
  Ritorna -1 se c'è un problema*/
int mr_token_result_pair_list_destroy(mr_token_result_pair_list_t* l);
#endif
