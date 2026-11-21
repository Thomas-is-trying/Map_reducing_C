#ifndef MR_PROTOCOL_UTILITIES_H
#define MR_PROTOCOL_UTILITIES_H

#include <stddef.h>
#include <sys/types.h>

/* Limite massimo (in byte) accettato per qualunque lunghezza letta da un
 * header del protocollo (nome file, contenuto riga, token, value, result).
 */
#define MR_MAX_MSG_LEN (16 * 1024 * 1024) /* 16 MB */

/*
 * Legge esattamente n byte su fd, gestendo letture
 * parziali e interruzioni da segnale (EINTR)
 *
 * Ritorno:
 *   n    tutti gli n byte richiesti sono stati scritti;
 *   -1   errore 
 */
ssize_t writen(int fd, const void *buf, size_t n);

/* *scrive esattamente n byte su fd, gestendo scritture
 * parziali e interruzioni da segnale (EINTR).
 *
 * Ritorno:
 *   n    tutti gli n byte richiesti sono stati letti/scritti;
 *   0    EOF raggiunto immediatamente, senza aver letto
 *        alcun byte: indica la fine pulita del flusso;
 *   m    con 0 < m < n  EOF raggiunto a metà messaggio:
 *        errore di protocollo, il messaggio è incompleto;
 *   -1   errore.
 */
ssize_t readn(int fd, void *buf, size_t n);


/*
 * Header del messaggio "riga di file" (processo principale -> mapper).
 * Seguito sulla pipe da file_name_len byte (nome file) e poi da
 * line_length byte (contenuto della riga, senza terminatore \n).
 */
typedef struct {
    size_t file_name_len;
    size_t line_number;
    size_t line_length;
} mr_line_header_t;


/*
 * Header di una coppia <token, processed_token> (mapper -> reducer).
   Seguito da token_len byte (il token,solo caratteri alfanumerici ASCII) e poi
    da value_len byte (il value,dato opaco, byte arbitrari).
 */
typedef struct {
    size_t token_len;
    size_t value_len;
} mr_pair_header_t;

/*
   Header di un record di output (reducer -> processo principale).
   seguito da token_len byte (il token) e poi da result_len byte
  (il risultato, dato opaco prodotto dal reducer).
 */
typedef struct {
    size_t token_len;
    size_t result_len;
} mr_result_header_t;

/*Funzione per mandare un (mr_line_header_t + nome file  + contenuto della riga)(processo principale -> mapper) usando written()
  Ritorna 0 dopo aver scritto su fd (mr_line_header_t + nome file  + contenuto della riga)
  Ritorna -1 se c'è un problema*/
int send_line_msg(int fd, const char *file_name, size_t file_name_len,unsigned long line_number, const char *line, size_t line_len);

/*Funzione per mandare un (mr_pair_header_t + il token +il value)(mapper -> reducer) usando written()
  Ritorna 0 dopo aver scritto su fd (mr_pair _header_t + il token +il value)
  Ritorna -1 se c'è un problema*/
int send_pair_msg(int fd, const char *token, const void *value, size_t value_len);

/*Funzione per mandare un (mr_result_header_t + il token + il risultato)(reducer -> processo principale) usando written()
  Ritorna 0 dopo aver scritto su fd (mr_result_header_t + il token + il risultato)
  Ritorna -1 se c'è un problema*/
int send_result_msg(int fd, const char *token, const void *result, size_t result_len);


/*
 * token/file_name vengono restituiti come stringhe C valide, terminate
 * da '\0' (il terminatore non fa parte della lunghezza logica, che
 * viene comunque restituita separatamente). value/result NON vengono
 * terminati da '\0': sono dati opachi e vanno sempre trattati insieme
 * alla relativa lunghezza, mai con funzioni come strlen()/strcmp().
 */

/*Funzione per ricevere un (mr_line_header_t + nome file  + contenuto della riga)(processo principale -> mapper) usando readn()
  Alloca dinamicamente i buffer di output: èresponsabilità del chiamante liberarli con free() una volta finito di usarli.
  Ritorna 0 dopo aver letto su fd (mr_line_header_t + nome file  + contenuto della riga) e aver assegnato alle variabili passate il suo contenuto
  Ritorna 1 EOF pulito: non è arrivato nessun messaggio, il flusso è terminato normalmente (nessun parametro di output è valido);
  Ritorna -1 se c'è un problema*/
int recv_line_msg(int fd, char **file_name, size_t *file_name_len,unsigned long *line_number, char **line, size_t *line_len);

/*Funzione per ricevere un (mr_pair_header_t + il token +il value)(mapper -> reducer)  usando readn()
  Alloca dinamicamente i buffer di output: èresponsabilità del chiamante liberarli con free() una volta finito di usarli.
  Ritorna 0 dopo aver letto su fd (mr_pair _header_t + il token +il value)  e aver assegnato alle variabili passate il suo contenuto
  Ritorna 1 EOF pulito: non è arrivato nessun messaggio, il flusso è terminato normalmente (nessun parametro di output è valido);
  Ritorna -1 se c'è un problema*/
int recv_pair_msg(int fd, char **token, void **value, size_t *value_len,size_t *token_len);

/*Funzione per ricevere un (mr_result_header_t + il token + il risultato)(reducer -> processo principale) usando readn()
  Alloca dinamicamente i buffer di output: èresponsabilità del chiamante liberarli con free() una volta finito di usarli.
  Ritorna 0 dopo aver letto su fd (mr_result_header_t + il token + il risultato) e aver assegnato alle variabili passate il suo contenuto
  Ritorna 1 EOF pulito: non è arrivato nessun messaggio, il flusso è terminato normalmente (nessun parametro di output è valido);
  Ritorna -1 se c'è un problema*/
int recv_result_msg(int fd, char **token, void **result, size_t *result_len,size_t *token_len);

#endif