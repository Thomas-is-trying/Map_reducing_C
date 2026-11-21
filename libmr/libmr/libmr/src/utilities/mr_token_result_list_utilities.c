#include "mr_token_result_list_utilities.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>

int mr_token_result_pair_list_init(mr_token_result_pair_list_t* l){
    //controlla che il puntatore l non sia NULL
    if(l == NULL){errno = EINVAL; return -1;}
    // iniziallizza le altre variabili
    l->capacity = MR_TOKEN_RESULT_LIST_START_CAPACITY;
    l->count = 0;
    // fa una malloc per mr_token_result_pair_t* quindi arr è un puntatore con spazio [[mr_token_result_pair_t],*MR_TOKEN_RESULT_LIST_START_CAPACITY]
    l->arr = (mr_token_result_pair_t*)malloc(sizeof(mr_token_result_pair_t)*MR_TOKEN_RESULT_LIST_START_CAPACITY);
    if(l->arr== NULL){return -1;}
    return 0;
}
int mr_token_result_pair_list_add_realloc(mr_token_result_pair_list_t* l, mr_token_result_pair_t tr){
    //controlla che il puntatore l non sia NULL 
   if(l == NULL){errno = EINVAL; return -1;}
    if (l->count == l->capacity)
    {   
        //creo un un array con realloc(copio il contenuto di arr e aggiungo altro spazio)
        mr_token_result_pair_t* temp = realloc(l->arr,(sizeof(mr_token_result_pair_t)*l->capacity*MR_TOKEN_RESULT_LIST_GROW_FACTOR));
        if(temp == NULL){return -1;}
        //assegno ad arr il puntatore all'array appena creato
        l->arr = temp;
        // aggiorno la variabile capacity perchè rappresenti effettivamente la capacity di l
        l->capacity *= MR_TOKEN_RESULT_LIST_GROW_FACTOR;
    }
     //aggiungo tr a l
    l->arr[l->count] = tr;
    //aggiorno il numero di elementi presenti all'interno di l
    l->count++;
    return 0;
}


static int  cmp(const void* a,const void* b){
    //eseguo i cast per rappresentare i puntatori ai mr_token_result_pair_t
    const mr_token_result_pair_t* A = (const mr_token_result_pair_t*) a;
    const mr_token_result_pair_t* B = (const mr_token_result_pair_t*) b;
     //uso strcmp  come cmp sui token
    return strcmp(A->token,B->token);
}

int mr_token_result_pair_list_reorder(mr_token_result_pair_list_t* l){
    //controlla che il puntatore l sia null
    if(l == NULL){errno = EINVAL; return -1;}
    //usa qsort su l->arr com cmp che usa strcmp
    qsort(l->arr,l->count,sizeof(mr_token_result_pair_t),cmp);
    return 0;
}

int mr_token_result_pair_list_destroy(mr_token_result_pair_list_t* l){
    //controlla che il puntatore a l non sia NULL
    if(l == NULL){errno = EINVAL; return -1;}
    //libera il guscio( gli elementi sono già stati liberati dal reducer_process_main)
    free(l->arr);
    //evita un problemi nel caso di doppio free
    l->arr = NULL;
    return 0;
}