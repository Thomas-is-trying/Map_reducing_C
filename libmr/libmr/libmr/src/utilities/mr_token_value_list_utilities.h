#ifndef MR_TOKEN_VALUE_LIST_UTILITIES_H
#define MR_TOKEN_VALUE_LIST_UTILITIES_H
#include "../../include/mr.h"

#include <sys/types.h>

//capacity iniziale per mr_token_values_list_t
#define MR_TOKEN_VALUES_LIST_START_CAPACITY 1
//fattore di crescita per la realloc
#define MR_TOKEN_VALUES_GROW_FACTOR 2

//capacity iniziale per mr_token_values_list
#define MR_TOKENS_AND_VALUES_LIST_START_CAPACITY 1
//fattore di crescita per la realloc
#define MR_TOKENS_AND_VALUES_LIST_GROW_FACTOR 2



/*struct per contenere un pair di [token,value]*/
typedef struct {
    char* token;
    size_t token_len;
    //mr_value_t contiene data e la grandezza di data
    mr_value_t value;
} mr_token_value_pair_t;

/*struct per contenere un pair di [token,values[]]
  usato per il raggruppamento*/
typedef struct {
    size_t capacity;
    size_t count;
    // token associato all'array di values prodotti dal mapper
    char* token;
    size_t token_len;
    //array di mr_value_t con lo stesso token
    mr_value_t* arr;
} mr_token_values_list_t;

/*struct per contenere una lista di pair[token,values[]]*/
typedef struct {
    size_t capacity;
    size_t count;
    mr_token_values_list_t* arr;
} mr_tokens_and_values_list_t;


/*Funzione per inizializzare mr_tokens_and_values_list_t con MR_TOKEN_AND_VALUES_LIST_START_CAPACITY
  Ritorna 0 dopo aver settato tutti i campi di mr_tokens_and_values_list_t,
  Ritorna -1 se c'è un problema*/
int mr_tokens_and_values_init(mr_tokens_and_values_list_t* l);


/*Funzione per aggiungere un mr_value_t v a mr_token_values_list_t* l, 
  utilizza realloc(con MR_TOKEN_VALUES_GROW_FACTOR) e riassegna il puntatore a l->arr
  Ritorna 0 dopo aver aggiunto v a l,
  Ritorna -1 se c'è un problema*/
int mr_token_values_list_add_realloc(mr_token_values_list_t* l, mr_value_t v);

/*Funzione per aggiungere un mr_token_value_pair_t tv a mr_tokens_and_values_list_t* l, 
  utilizza realloc(con MR_TOKENS_AND_VALUES_LIST_GROW_FACTOR) e riassegna il puntatore a l->arr
  Ritorna 0 dopo aver aggiunto tv a l,
  Ritorna -1 se c'è un problema*/
int mr_tokens_and_values_add_realloc(mr_tokens_and_values_list_t* l, mr_token_values_list_t* tvs );

/*Funzione per liberare mr_tokens_and_values_list_t* l,ne libera gli elementi e poi ne libera
 il guscio interno per ogni elemento e poi quello esterno finale
  Ritorna 0 dopo aver liberato l
  Ritorna -1 se c'è un problema*/
int mr_tokens_and_values_destroy(mr_tokens_and_values_list_t* l);

/*Funzione per orchestrare il raggruppamento di mr_token_value_pair_t,
  cerca se il gruppo mr_token_values_list_t associato al token di tv esiste già
  nel caso esistesse lo aggiunge a quel gruppo, se non esiste crea un nuovo
  mr_token_values_list_t e lo aggiunge a mr_token_values_list_t

  Ritorna 0 dopo aver aggiunto tv ad un mr_token_values_list_t o dopo aver aggiunto un nuovo mr_token_values_list_t con il token di tv
  Ritorna -1 se c'è un problema*/
int mr_tokens_and_values_add(mr_tokens_and_values_list_t* l, mr_token_value_pair_t tv);

/*Funzione ausiliare per cercare il token associato a mr_token_value_pair_t tv 
  in mr_tokens_and_values_list_t* l
  Ritorna n indice del mr_token_values_list_t associato a mr_token_value_pair_t tv
  Ritorna -1 se non trova il token*/
int mr_token_value_find_group(mr_tokens_and_values_list_t* l, mr_token_value_pair_t tv);

/*Funzione ausiliare per creare un gruppo mr_token_values_list_t da mr_token_value_pair_t tv 
 il gruppo avrà forma {tv.token,value[tv.value]}
  Ritorna un puntatore a mr_token_values_list_t* creato;
  Ritorna NULL se c'è un problema*/
mr_token_values_list_t* mr_token_values_make_group(mr_token_value_pair_t tv);

#endif