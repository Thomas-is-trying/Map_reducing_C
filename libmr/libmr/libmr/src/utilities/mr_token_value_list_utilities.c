#include "mr_token_value_list_utilities.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <sys/stat.h>

int mr_tokens_and_values_init(mr_tokens_and_values_list_t* l){
        /*controllo che il puntatore l sia valido*/
    if(l == NULL){errno = EINVAL; return -1;}
     // iniziallizza le altre variabili
    l->capacity = MR_TOKENS_AND_VALUES_LIST_START_CAPACITY;
    l->count = 0;
    // fa una malloc per mr_token_values_list_t* quindi arr è un puntatore con spazio [[mr_token_values_list_t],* MR_TOKENS_AND_VALUES_LIST_START_CAPACITY]
    l->arr = (mr_token_values_list_t*)malloc(sizeof(mr_token_values_list_t)*MR_TOKENS_AND_VALUES_LIST_START_CAPACITY);
    if(l->arr== NULL){return -1;}
    return 0;
}

int mr_token_values_list_add_realloc(mr_token_values_list_t* l, mr_value_t v){
    //controlla che il puntatore l non sia NULL
    if(l == NULL){errno = EINVAL; return -1;}
    //controllo se ho finito lo spazio
    if (l->count == l->capacity)
    {   
         //creo un un array con realloc(copio il contenuto di arr e aggiungo altro spazio)
        mr_value_t* temp = realloc(l->arr,(sizeof(mr_value_t)*l->capacity*MR_TOKEN_VALUES_GROW_FACTOR));
        if(temp == NULL){return -1;}

        //assegno ad arr il puntatore all'array appena creato
        l->arr = temp;

        // aggiorno la variabile capacity perchè rappresenti effettivamente la capacity di l
        l->capacity *= MR_TOKEN_VALUES_GROW_FACTOR;
    }
    //aggiungo v a l copiandone i dati nel nuovo elemento
    l->arr[l->count] = v;
    //aggiorno il numero di elementi presenti all'interno di l
    l->count++;
    return 0;
}
int mr_tokens_and_values_add_realloc(mr_tokens_and_values_list_t* l, mr_token_values_list_t* tvs){
    //controlla che i puntatori tvs e non siano NULL
    if(l == NULL || tvs == NULL){errno = EINVAL; return -1;}
    //controllo se ho finito lo spazio
    if (l->count == l->capacity)
    {   //creo un un array con realloc(copio il contenuto di arr e aggiungo altro spazio)
        mr_token_values_list_t* temp = realloc(l->arr,(sizeof(mr_token_values_list_t)*l->capacity*MR_TOKENS_AND_VALUES_LIST_GROW_FACTOR));
        if(temp == NULL){return -1;}
        //assegno ad arr il puntatore all'array appena creato
        l->arr = temp;
        // aggiorno la variabile capacity perchè rappresenti effettivamente la capacity di l
        l->capacity *= MR_TOKENS_AND_VALUES_LIST_GROW_FACTOR;
    }
    //aggiungo tvs a l copiandone i dati nel nuovo elemento
    l->arr[l->count] = *tvs;
    //aggiorno il numero di elementi presenti all'interno di l
    l->count++;
    return 0;
}

int mr_tokens_and_values_destroy(mr_tokens_and_values_list_t* l){
    //controlla che il puntatore a l non sia NULL
    if(l == NULL){errno = EINVAL; return -1;}
    //libera tutti i componenti
    for (size_t i = 0; i < l->count; i++)
    {
        //usa la variabile group per iterare su VALUES e liberarne il token
        mr_token_values_list_t* group = &(l->arr[i]);
        free(group->token);
        for (size_t j = 0; j < group->count; j++)
        {
            //void silenzia gli warnings che forse non dovevo silenziare
            free((void*)group->arr[j].data);
        }
        //libera il guscio interno
        free(group->arr);
    }
    //libera il guscio esterno
    free(l->arr);
    //evita doppio free
    l->arr = NULL;
    l->count = 0;
    l->capacity = 0;
    return 0;
}

int mr_tokens_and_values_add(mr_tokens_and_values_list_t* l, mr_token_value_pair_t tv ){
    // controllo che l non punti a NULL
    if (l== NULL){errno = EINVAL;return -1;}
    //guardo se tv può essere inserito in uno degli elementi di l e nel caso ne scopro l'indice
    int idx = mr_token_value_find_group(l,tv);
    //se non lo trovo allora creo un nuovo elemento di l e lo aggiungo
    if( idx != -1){
        //uso l'indice che ho trovato per inserirlo nell'elemento con il token corrispondente
       if(mr_token_values_list_add_realloc(&(l->arr[idx]),tv.value) == -1){free((void*)tv.token);free((void*)tv.value.data); return -1;};
        free(tv.token);
    }   
    else{
        //se non lo trovo allora creo un nuovo elemento di l e lo aggiungo
        mr_token_values_list_t* temp = mr_token_values_make_group(tv);
        if(temp == NULL){free((void*)tv.token);free((void*)tv.value.data);return -1;}
        if(mr_tokens_and_values_add_realloc(l,temp)== -1){free((void*)temp->token);free((void*)temp->arr[0].data);free(temp->arr);free(temp);return -1;}
        free(temp);}
        return 0;
}


int mr_token_value_find_group(mr_tokens_and_values_list_t* l, mr_token_value_pair_t tv){
    // itera su tutti gli elementi di mr_tokens_and_values_list_t* l
    for (size_t i = 0; i < l->count; i++)
    {   /* usa strcmp per vedere se il token di tv è già associato ad un elemento di l 
        se sì ritorna l'indice dell'elemento mr_token_values_list_t*/
        if(!strcmp(l->arr[i].token,tv.token)){
            return i;
        }
    }
    //se non lo trova ritorna -1
    return -1;
}

mr_token_values_list_t* mr_token_values_make_group(mr_token_value_pair_t tv){
    //alloco lo spazio per il mr_token_values_list_t
    mr_token_values_list_t* temp = (mr_token_values_list_t*)malloc(sizeof(mr_token_values_list_t));
    if(temp == NULL){return NULL;}
    //il gruppo inizia con MR_TOKEN_VALUES_LIST_START_CAPACITY
    temp->capacity = MR_TOKEN_VALUES_LIST_START_CAPACITY;
    //  l'elemento che viene usato per creare il gruppo
    temp->count = 1;
    //alloco lo spazio per il mr_token_values_list_t per l'array di values e ne salvo il puntatore
    temp->arr = (mr_value_t*)malloc(sizeof(mr_value_t)*MR_TOKEN_VALUES_LIST_START_CAPACITY);
    if(temp->arr == NULL){free(temp);return NULL;}

    //copio le variabili di tv in temp
    temp->token = tv.token;
    temp->token_len = tv.token_len;
    temp->arr->data = tv.value.data;
    temp->arr->size = tv.value.size;
    return temp;
}