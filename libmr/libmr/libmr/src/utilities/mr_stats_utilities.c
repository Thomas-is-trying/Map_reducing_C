#include "mr_stats_utilities.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

int mr_stats_sniff_log(const char* path, mr_log_ctx_t* log_ctx, mr_stats_t *stats){
    if((log_ctx == NULL) || (stats == NULL)){errno = EINVAL; return -1;}
    //se il path è NULL faccio una chiamata ricorsiva ed uso il nome di default
    if(path == NULL){
        if(mr_stats_sniff_log(LOG_DEFAULT_NAME, log_ctx, stats) == -1){return -1;};
        return 0; 
    }
    //apro lo stream del file in modalità lettura
    FILE *f = fopen(path, "r");
    if(f == NULL){return -1;}
    /*dato che il file log può non essere unico per ogni run del sistema,
    uso il puntatore all'inizio della specifica run per cui sto calcolando le stats*/
    if(fseek(f, log_ctx->run_start_offset, SEEK_SET) == -1){fclose(f); return -1;}
    // buffer di appoggio con grandezza  MAX_SIZE_LINE_LOG
    char line[MAX_SIZE_LINE_LOG];
    //scorro linea per linea
    while (fgets(line, sizeof(line), f)) {
        //variabile di appoggio per il pattern matching
        size_t val;
        //strstr trova la prima occorrenza di una stringa all'interno di un altra stringa e ne ritorna il puntatore
        //strchr trova la prima occorrenza di un char all'interno di una stringa e ne ritorna il puntatore
        //sscanf legge una stringa formattata e associa il valore che riesce a trovare


        /*controllo che la stringa che sto cercando sia all'interno di quella che sto guardando
            se sì allora faccio una scanf partendo dal puntatore a : in line,
             aggiungo 1 per andare subito dopo e faccio un match con %zu per size_t*/
        if (strstr(line, "PAIRS_PRODUCED") && sscanf(strrchr(line, ':') + 1, "%zu", &val) == 1) {
            stats->pairs_produced = val;
        }
        if (strstr(line, "DISTINCT_TOKENS") && sscanf(strrchr(line, ':') + 1, "%zu", &val) == 1) {
            stats->distinct_tokens = val;
        }
        if (strstr(line, "RESULTS_EMITTED") && sscanf(strrchr(line, ':') + 1, "%zu", &val) == 1) {
            stats->results_emitted = val;
        }
    }
    fclose(f);
    return 0;
}


int mr_stats_file_to_zero( mr_stats_t* stats){
    // controllo che il puntatore sia valido
    if(stats == NULL){errno = EINVAL; return -1;}
    //azzero le statistiche
    stats->distinct_tokens = 0;
    stats->elapsed_seconds = 0.0;
    stats->lines_read = 0;
    stats->pairs_produced = 0;
    stats->results_emitted = 0;
    return 0;
}

int mr_stats_write_file(const char *path, const mr_stats_t *stats){
    //controllo che i vari puntatori siano validi (il fatto che path non sia null,e quindi stats voluto, è già controllato dalla chiamata in mr.c)
    if((path==NULL) || (stats == NULL)){errno = EINVAL; return -1;}
    //apro lo stream dello stats file in modalità lettura
    FILE* stats_file = fopen(path,"w");
    if(stats_file == NULL){return -1;}
    //scrivo in maniera formattata i vari atributi di mr_stats_t con fprintf, e ne verifico la scrittura
    if((fprintf(stats_file,"LE STATISTICHE\nElapsed_seconds: %f\nLines_read: %zu\nPairs_produced: %zu\nDistinct_tokens: %zu\nResults_emitted: %zu",stats->elapsed_seconds,stats->lines_read,stats->pairs_produced,stats->distinct_tokens,stats->results_emitted))<0)
    {fclose(stats_file);return -1;}
    //chiudo lo stream dello stats file 
    fclose(stats_file);
    
    return 0;
}

