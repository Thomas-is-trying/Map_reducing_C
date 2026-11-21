#include "mr_hash_utilities.h"
/* [ADDENDUM] Funzione di hashing di default, usata se attr->hash == NULL.
 * Semplice e deterministica (008 - djb2 hash).
 * reducer_process.c può usarla per assegnare i token ai thread reducer. */
size_t mr_default_hash(const char *token, size_t token_len, void *user_arg) {
    (void)user_arg;
    //valore di dbj2 per convenzione 
    size_t h = 5381;
    //itera tokern per token_len
    for (size_t i = 0; i < token_len; i++) {
        //moltiplica hash per 33 e aggiunge il valore ASCII del carattere token[i]
        h = ((h << 5) + h) + (unsigned char)token[i]; /* h*33 + c */
    }
    return h;
}