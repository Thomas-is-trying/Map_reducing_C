//chiedo al preprocessore di sbloccare le funzionalità posix per avere un timer più preciso di time.h
#define _POSIX_C_SOURCE 200809L

#include "mr_internal.h"
#include "mr_log.h"


#include "utilities/mr_protocol_utilities.h"
#include "utilities/mr_path_list_utilities.h"
#include "utilities/mr_stats_utilities.h"
#include "utilities/mr_attr_utilities.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>

int mr_create(mr_t *mr,const mr_attr_t *attr,mr_mapper_t mapper,mr_reducer_t reducer,void *user_arg) {
    // contrllo che le variabili siano passate correttamente 
    if(mr == NULL||attr == NULL||mapper == NULL||reducer == NULL){errno = EINVAL; return -1;}
    // alloco lo spazio per una struct mr(carattere opaco)
    mr_t mr_l = malloc(sizeof(struct mr));
    if(mr_l==NULL){return -1;}
    //inizializzo l'attributo attr
    if(mr_attr_init(&mr_l->attr )== -1){free(mr_l);return -1;}; 
    //setto le variabili per ,mr_l attr da *attr che mi è stato passato
    if(mr_attr_set_log_file(&mr_l->attr,attr->log_file) == -1){mr_attr_destroy(&mr_l->attr);free(mr_l); return -1;};
    if(mr_attr_set_stats_file(&mr_l->attr,attr->stats_file) == -1){mr_attr_destroy(&mr_l->attr);free(mr_l); return -1;};
    if(mr_attr_set_hash_function(&mr_l->attr,attr->hash,attr->hash_arg) == -1){mr_attr_destroy(&mr_l->attr);free(mr_l); return -1;};
    if(mr_attr_set_mapper_threads(&mr_l->attr,attr->mapper_threads) == -1){mr_attr_destroy(&mr_l->attr);free(mr_l); return -1;};
    if(mr_attr_set_reducer_threads(&mr_l->attr,attr->reducer_threads) == -1){mr_attr_destroy(&mr_l->attr);free(mr_l); return -1;};
    if(mr_attr_set_queue_size(&mr_l->attr,attr->queue_size) == -1){mr_attr_destroy(&mr_l->attr);free(mr_l); return -1;};

    // passo le variabili che rappresentano le funzioni di mapping e reducing
    mr_l->mapper = mapper;
    mr_l->reducer = reducer;

    //azzero le variabili per le stats[addeddum]
    mr_stats_file_to_zero(&(mr_l->stats));

    //non controllo  che sia diverso da NULL perchè lo user potrebbe non avere bisogno di altro context
    mr_l->user_arg = user_arg;

    // setto la variabile mr come puntatore alla memoria puntata da mr_l
    *mr = mr_l;
    return 0;
}


int mr_start(mr_t mr, const char *input_path, const char *output_path) {

    //faccio partire il timer per le stats
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    


    //inizializzo il logging
    if(mr_log_init(mr->attr.log_file,&(mr->mr_log_ctx))){return -1;};

    //[LOG] segnalo l'inizzializzazione del logging
    mr_log_write(&(mr->mr_log_ctx), "main", "LOG_INIT", "-", "log inizializzato con successo");

    //creo gli array per gestire gli fd delle pipes
    int main_to_mapper[2];
    int mapper_to_reducer[2];
    int reducer_to_main[2];

    //creo le varie pipe e verifico 
    if (pipe(main_to_mapper) == -1){return -1;}
    if (pipe(mapper_to_reducer) == -1){ return -1;}
    if (pipe(reducer_to_main) == -1){return -1;}

    //[LOG] segnalo al creazione delle pipe
    mr_log_write(&(mr->mr_log_ctx), "main", "PIPES_CREATED", "-", "create 3 pipe: main->mapper, mapper->reducer, reducer->main");


    //faccio copie locali dell'input path e output path per rispettare le variabili const
    char* input_path_l;
    if((input_path_l = (char*)malloc(strlen(input_path)+1))==NULL){return -1;};
    strcpy(input_path_l,input_path);

    char* output_path_l;
    if((output_path_l = (char*)malloc(strlen(output_path)+1))==NULL){return -1;}
    strcpy(output_path_l,output_path);

    //faccio un fork() per il mapper
    pid_t mapper = fork();

    if (mapper == -1){
    close(mapper_to_reducer[0]);
    close(mapper_to_reducer[1]);
    close(main_to_mapper[0]);
    close(main_to_mapper[1]);
    close(reducer_to_main[1]);
    close(reducer_to_main[0]);
    free(input_path_l);free(output_path_l);
    return -1;
}
    if (mapper == 0)
    {   //chiudo gli fd che non mi servono nel mapper

        //prima quelle dal reducer al main
        close(reducer_to_main[0]);
        close(reducer_to_main[1]);

        //dup2 ridefinisce cosa significa STDIN_FILENO per questo processo

        //chiudo la pipe per la scrittura su main e connetto la pipe da lettura a main allo stdin
        close(main_to_mapper[1]);
        dup2(main_to_mapper[0],STDIN_FILENO);
        close(main_to_mapper[0]);

        //chiudo la pipe per la lettura da reducer e connetto la pipe da mapper a reducer allo stdout
        close(mapper_to_reducer[0]);
        dup2(mapper_to_reducer[1],STDOUT_FILENO);
        close(mapper_to_reducer[1]);
        

        //attivo il processo di mapping , che userà internamente stdin e stdout
        mapper_process_main(mr);
        //mapper_process_main chiude già stdout/fd_out al suo interno (una sola volta,dopo aver scritto tutti i risultati)
        //chiudo il log locale
        mr_log_close(&(mr->mr_log_ctx));

        _exit(0);
    }
    //[LOG] segnalo la creazione  del processo MAPPER
    mr_log_write(&(mr->mr_log_ctx), "main", "PROCESS_CREATED", "-", "processo mapper creato con pid %d", mapper);
    
    pid_t reducer = fork();
    if (reducer == -1){
        close(mapper_to_reducer[0]);
        close(mapper_to_reducer[1]);
        close(main_to_mapper[0]);
        close(main_to_mapper[1]);
        close(reducer_to_main[1]);
        close(reducer_to_main[0]);
        waitpid(mapper,NULL,0);
        free(input_path_l);free(output_path_l);
        return -1;
    }
    if (reducer == 0)
    {   //chiudo gli fd che non mi servono nel mapper
        //prima quelle dal main al mapper
        close(main_to_mapper[0]);
        close(main_to_mapper[1]);

        //chiudo la pipe per la scrittura su mapper e connetto la pipe per lettura da mapper allo stdin
        close(mapper_to_reducer[1]);
        dup2(mapper_to_reducer[0],STDIN_FILENO);
        close(mapper_to_reducer[0]);

         //chiudo la pipe per la lettura da main e connetto la pipe da reducer a main allo stdout
        close(reducer_to_main[0]);
        dup2(reducer_to_main[1],STDOUT_FILENO);
        close(reducer_to_main[1]);

        //attivo il processo di reducing , che userà internamente stdin e stdout
        reducer_process_main(mr);

        //reducer_process_main chiude già stdout/fd_out al suo interno (una sola volta,dopo aver scritto tutti i risultati)
        mr_log_close(&(mr->mr_log_ctx));
        _exit(0);
    }

    //[LOG] segnalo la creazione  del processo REDUCER
    mr_log_write(&(mr->mr_log_ctx), "main", "PROCESS_CREATED", "-", "processo reducer creato con pid %d", reducer);
    

    //chiudo gli fd inutili nel processo principale
    close(mapper_to_reducer[0]);
    close(mapper_to_reducer[1]);
    close(main_to_mapper[0]);
    close(reducer_to_main[1]);
    
    //creo una lista per contenere il path o tutti i path
    mr_path_list_t paths_list;

    //creo un puntatore a quella lista
    mr_path_list_t* ptr_to_paths_list = &paths_list;

    //inizializzo la lista per i path
    if(mr_path_list_init(ptr_to_paths_list)){free(input_path_l);free(output_path_l);close(main_to_mapper[1]); close(reducer_to_main[0]);waitpid(mapper,NULL,0);waitpid(reducer,NULL,0);return -1;};

    //popolo la lista (scansione ricorsiva: file singolo o directory con sottocartelle)
    if(mr_path_scan_recursive(input_path_l,ptr_to_paths_list)){mr_path_list_destroy(ptr_to_paths_list);free(input_path_l);free(output_path_l); close(main_to_mapper[1]); close(reducer_to_main[0]);waitpid(mapper,NULL,0);waitpid(reducer,NULL,0);return -1;}

    //la riordino lessicograficamente, per il determinismo dell'ordine di lettura
    if(mr_path_list_reorder(ptr_to_paths_list)){mr_path_list_destroy(ptr_to_paths_list);free(input_path_l);free(output_path_l); close(main_to_mapper[1]); close(reducer_to_main[0]);waitpid(mapper,NULL,0);waitpid(reducer,NULL,0);return -1;}
    
    //mando il contenuto dei file diviso in linee e loggo l'apertura e la chiusura degli input
    if(mr_path_list_send_lines_to_mapper(ptr_to_paths_list,send_line_msg,main_to_mapper[1],&(mr->mr_log_ctx))){mr_path_list_destroy(ptr_to_paths_list);free(input_path_l);free(output_path_l); close(main_to_mapper[1]); close(reducer_to_main[0]);waitpid(mapper,NULL,0);waitpid(reducer,NULL,0);return -1;}
    //[LOG] segnalo  l'invio delle righe al mapper, total_line_count usato anche per  stats
    mr_log_write(&(mr->mr_log_ctx), "main", "LINES_SENT", "-", "righe inviate al mapper: %zu", ptr_to_paths_list->total_line_count);
    mr->stats.lines_read =  ptr_to_paths_list->total_line_count;

    //segnalo al thread reader del mapper process che ho finito di inserire input
    close(main_to_mapper[1]);

    //libero la memoria della lista
    mr_path_list_destroy(ptr_to_paths_list);

    //ricevo il risultato dal reader(logging interno)
    if(mr_receive_from_reducer(recv_result_msg,reducer_to_main[0],output_path_l,&(mr->mr_log_ctx))){free(input_path_l);free(output_path_l);close(reducer_to_main[0]);waitpid(mapper,NULL,0);waitpid(reducer,NULL,0);return -1;};
    close(reducer_to_main[0]);

    //libero input_path_l e il fratellastro
    free(input_path_l);
    free(output_path_l);

    /*aspetto la terminazione di entrambi i figli: senza waitpid resterebbero zombie
    (il kernel non libera la loro entry nella process table finché nessuno "raccoglie" lo stato di uscita)*/
    waitpid(mapper,NULL,0);
    waitpid(reducer,NULL,0);

    /*scrivo i risultati delle stats,rileggo dal log i contatori che mapper/reducer hanno scritto durante l'elaborazione
    (pairs_produced, distinct_tokens, results_emitted)*/
    if(mr_stats_sniff_log(mr->attr.log_file,&(mr->mr_log_ctx),&(mr->stats))==-1){return -1;};
    //"fermo" il timer
    clock_gettime(CLOCK_MONOTONIC, &end);
    //calcolo
    mr->stats.elapsed_seconds = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    if(!(mr->attr.stats_file == NULL)){
        if(mr_stats_write_file(mr->attr.stats_file,&(mr->stats)) == -1){return -1;};
    }

    //[LOG]Segnalo la chiusura del main
    mr_log_write(&(mr->mr_log_ctx), "main", "PROCESS_END", "-", "elaborazione completata con successo");
    return 0;
}

int mr_destroy(mr_t mr) {
    if(mr == NULL){errno = EINVAL;return -1;}
    sem_unlink(mr->mr_log_ctx.sem_name);
    mr_attr_destroy(&(mr->attr));
    mr_log_close(&(mr->mr_log_ctx));
    free(mr);
    return 0;
}