#include "mr_attr_utilities.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

int mr_attr_init(mr_attr_t *attr) {
    /*controllo che il puntatore attr sia valido*/
    if(!attr){errno = EINVAL; return -1;}
    //inizializzo mapper,reducer e queue con i valori default
    attr->mapper_threads = MR_DEFAULT_MAPPER_THREADS;
    attr->reducer_threads = MR_DEFAULT_REDUCER_THREADS;
    attr->queue_size = MR_DEFAULT_QUEUE_SIZE;
    //inizializzo i puntatori a stats file(non in uso) e log file(usa il default)
    attr->stats_file = NULL;
    attr->log_file = NULL;
    //default di hash e hash_argument, utilizzerà dbj2
    attr->hash = NULL;
    //con hash_argument null la hash function di defautl fa return 0
    attr->hash_arg = NULL;
    return 0;
}

int mr_attr_destroy(mr_attr_t *attr) {
    /*controllo che il puntatore attr sia valido*/
    if(!attr){errno = EINVAL; return -1;}
    //libero i due puntatori allocati e muta i warnings(erano copie non path originali)
    free((void*)attr->log_file);
    free((void*)attr->stats_file);
    //evito double free nel caso mr_attr_destroy venisse chiamata più volte
    attr->log_file = NULL;
    attr->stats_file = NULL;
    return 0;
}

int mr_attr_set_mapper_threads(mr_attr_t *attr, size_t n) {
    /*controllo che il puntatore attr sia valido e che n sia valido secondo le specifiche*/
   if(n<1 || !attr){errno = EINVAL; return -1;}
    // assegno a mapper_threads il nuovo n
    else{attr->mapper_threads = n;}
    return 0;
}

int mr_attr_set_reducer_threads(mr_attr_t *attr, size_t n) {
   /*controllo che il puntatore attr sia valido e che n sia valido secondo le specifiche*/
   if(n<1 || !attr){errno = EINVAL; return -1;}
   // assegno a reducer_threads il nuovo n
    else{attr->reducer_threads = n;}
    return 0;
}

int mr_attr_set_queue_size(mr_attr_t *attr, size_t n) {
   /*controllo che il puntatore attr sia valido e che n sia valido secondo le specifiche*/
   if(n<1 || !attr){errno = EINVAL; return -1;}
   // assegno a queue_size il nuovo n
    else{attr->queue_size = n;}
    return 0;
}


int mr_attr_set_log_file(mr_attr_t *attr, const char *path) {
   /*controllo che il puntatore attr sia valido*/
    if(!attr){errno = EINVAL; return -1;}
    /*nel caso il puntatore fosse già stato assegnato in precedenza lo libero
    per evitare errori di dangling pointers*/
    if (attr->log_file != NULL) {free((void*)attr->log_file);}
    if((path == NULL)){attr->log_file = NULL;return 0;} 
    //se invece il path è nullo lascio il default e faccio return
    if((path == NULL)){return 0;} 
    
    //creo un buffer e verifico la malloc
    char* buff = (char*)malloc(strlen(path)+1);
    if(buff == NULL){return -1;}
    //copio il contenuto di path all'interno di buff
    strcpy(buff,path);
    /*il puntantore di log_file ora punta alla memoria puntata da buff
    così che se la memoria puntata da path cambiasse non si rifletterebbe sulla copia locale*/ 
    attr->log_file = buff;

    return 0;
}

/* [ADDENDUM] */
int mr_attr_set_stats_file(mr_attr_t *attr, const char *path) {
   /*controllo che il puntatore attr sia valido*/
    if(!attr){errno = EINVAL; return -1;}
    /*nel caso il puntatore fosse già stato assegnato in precedenza lo libero
    per evitare errori di dangling pointers*/
    if (attr->stats_file != NULL) {free((void*)attr->stats_file);}
    if((path == NULL)){attr->stats_file = NULL;return 0;} 
    //creo un buffer e verifico la malloc
    char* buff = (char*)malloc(strlen(path)+1);
    if(buff == NULL){return -1;}
     //copio il contenuto di path all'interno di buff
    strcpy(buff,path);
    /*il puntantore di stats_file ora punta alla memoria puntata da buff
    così che se la memoria puntata da path cambiasse non si rifletterebbe sulla copia locale*/ 
    attr->stats_file = buff;
    return 0;
}

/* [ADDENDUM] */
int mr_attr_set_hash_function(mr_attr_t *attr, mr_hash_t hash, void *hash_arg) {
    /*, attr->hash_arg = hash_arg.
     * hash può essere NULL per tornare al default (documentalo). */

   /*controllo che il puntatore attr sia valido*/
    if(!attr){errno = EINVAL; return -1;}
    //se hash NULL allora assegno NULL e hash_arg == NULL
    if((hash == NULL)){attr->hash = NULL;attr->hash_arg = NULL;return 0;}
    //assegno funzione e arg
    attr->hash = hash;
    attr->hash_arg = hash_arg;

    return 0;
}

