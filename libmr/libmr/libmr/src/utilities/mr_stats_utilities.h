#ifndef STATS_UTILITIES_H
#define STATS_UTILITIES_H
#include <sys/types.h>
#include "../mr_log.h"


/*[ADDENDUM] Statistiche finali di un'elaborazione, da scrivere in un
  file separato al termine di mr_start(). Popolale via il log/i contatori*/
typedef struct {
    double elapsed_seconds;//settato con timer in mr_start
    size_t lines_read;//settato grazie ad una variabile in path_list_t
    size_t pairs_produced;
    size_t distinct_tokens;
    size_t results_emitted;
} mr_stats_t;


/*[ADDENDUM] Scrive *stats nel file indicato in formato testuale leggibile.*/

/*Funzione per azzerare tutti gli atributi di mr_stats,
  Ritorna 0 dopo aver settato tutti i campu a 0 
  Ritorna -1 se c'è un problema*/
int mr_stats_file_to_zero( mr_stats_t *stats);

/*Funzione per sniffare il contenuto di mr_log.txt e estrapolarne i dati per mr_stats_t tramite pattern matching,
  Ritorna 0 dopo aver settato i campi PAIRS_PRODUCED DISTINCT_TOKENS e RESULTS_EMITTED 
  Ritorna -1 se c'è un problema*/
int mr_stats_sniff_log(const char* path, mr_log_ctx_t* log_ctx, mr_stats_t *stats);

/*Funzione per scrivere tutti gli atributi di mr_stats nel file indicato da path(mr->attr.stats_file)
  Se path è NULL allora le stats non sono richieste
  Ritorna 0 dopo aver scritto tutti i dati in path
  Ritorna -1 se c'è un problema*/
int mr_stats_write_file(const char *path, const mr_stats_t *stats);
#endif
