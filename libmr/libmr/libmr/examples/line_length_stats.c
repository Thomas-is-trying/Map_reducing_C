#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "../include/mr.h"

/*
 * Esempio "line_length_stats".
 *
 * Per ogni token, calcola la lunghezza MINIMA e MASSIMA (in byte) fra
 * tutte le righe in cui quel token compare, su tutti i file di input.
 *
 * Motivazione (rispetto a wordcount.c):
 * - il value intermedio NON e' un semplice int: e' una piccola struct
 *   binaria opaca (lls_value_t), per verificare che il framework tratti
 *   davvero il value come sequenza di byte di lunghezza nota, senza
 *   assumere nulla sul suo contenuto (sez. 3 e sez. 10 della specifica);
 * - il reducer emette PIU' di un risultato per lo stesso token (min e
 *   max), per esercitare esplicitamente il caso "piu' emit per token"
 *   menzionato in sez. 8 della specifica;
 * - il result NON e' una stringa C: e' una struct binaria con un campo
 *   di tag (per distinguere min/max) e un valore size_t, cosi' la
 *   correttezza del framework non puo' dipendere dal fatto che i
 *   risultati siano leggibili come testo (sez. 8: "la correttezza del
 *   framework non deve dipendere dal fatto che i risultati siano
 *   stringhe C").
 *
 * Convenzione sull'ordine dei risultati per token : per un dato token, il
 * reducer emette SEMPRE prima il risultato con tag LLS_TAG_MIN, poi
 * quello con tag LLS_TAG_MAX. L'ordine e' deterministico perche' un
 * singolo thread reducer processa un singolo gruppo (token) per intero
 * in una sola chiamata a user_reducer, quindi le due emit avvengono in
 * sequenza all'interno della stessa invocazione, mai in parallelo fra
 * loro.
 */

/* Value intermedio: value opaco emesso dal mapper (mapper -> reducer). */
typedef struct {
    size_t line_len;
} lls_value_t;

/* Tag per distinguere min/max nel result finale, cosi' l'interpretazione
 * non dipende dall'ordine di emit ma e' esplicita nel byte stream. */
typedef enum {
    LLS_TAG_MIN = 0,
    LLS_TAG_MAX = 1
} lls_tag_t;

/* Result finale: result opaco emesso dal reducer (reducer -> output). */
typedef struct {
    lls_tag_t tag;
    size_t value;
} lls_result_t;

/*
 * Mapper: per ogni token alfanumerico nella riga, emette <token, lls_value_t{line->line_len}>.
 */
static int lls_mapper(const mr_file_line_t *line, mr_emit_pair_t emit,
                       void *emit_arg, void *user_arg) {
    (void)user_arg;
    lls_value_t v;
    v.line_len = line->line_len;

    size_t i = 0;
    while (i < line->line_len) {
        if (isalnum((unsigned char)line->line[i])) {
            size_t start = i;
            while (i < line->line_len && isalnum((unsigned char)line->line[i])) {
                i++;
            }
            size_t token_len = i - start;
            char token[token_len + 1];
            memcpy(token, &line->line[start], token_len);
            token[token_len] = '\0';

            if (emit(token, &v, sizeof(v), emit_arg) == -1) {
                return -1;
            }
        } else {
            i++;
        }
    }
    return 0;
}

/*
 * Reducer: riceve tutti i lls_value_t associati al token, calcola
 * lunghezza minima e massima, ed emette due lls_result_t distinti
 * (tag MIN poi tag MAX), come da convenzione documentata sopra.
 */
static int lls_reducer(const char *token, const mr_value_t *values,
                        size_t values_count, mr_emit_result_t emit,
                        void *emit_arg, void *user_arg) {
    (void)user_arg;

    if (values_count == 0) {
        return 0;
    }

    const lls_value_t *first = (const lls_value_t *)values[0].data;
    size_t min_len = first->line_len;
    size_t max_len = first->line_len;

    for (size_t i = 1; i < values_count; i++) {
        const lls_value_t *v = (const lls_value_t *)values[i].data;
        if (v->line_len < min_len) { min_len = v->line_len; }
        if (v->line_len > max_len) { max_len = v->line_len; }
    }

    lls_result_t r_min;
    r_min.tag = LLS_TAG_MIN;
    r_min.value = min_len;
    if (emit(token, &r_min, sizeof(r_min), emit_arg) == -1) {
        return -1;
    }

    lls_result_t r_max;
    r_max.tag = LLS_TAG_MAX;
    r_max.value = max_len;
    if (emit(token, &r_max, sizeof(r_max), emit_arg) == -1) {
        return -1;
    }

    return 0;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "uso: %s <input_dir_o_file> <output_file>\n", argv[0]);
        return 1;
    }

    mr_t mr;
    mr_attr_t attr;

    if (mr_attr_init(&attr) == -1) { perror("mr_attr_init"); return 1; }
    if (mr_attr_set_mapper_threads(&attr, 4) == -1) { perror("mapper_threads"); return 1; }
    if (mr_attr_set_reducer_threads(&attr, 4) == -1) { perror("reducer_threads"); return 1; }
    if (mr_attr_set_queue_size(&attr, 64) == -1) { perror("queue_size"); return 1; }
    if (mr_attr_set_log_file(&attr, "line_length_stats.log.txt") == -1) { perror("log_file"); return 1; }

    if (mr_create(&mr, &attr, lls_mapper, lls_reducer, NULL) == -1) {
        perror("mr_create");
        mr_attr_destroy(&attr);
        return 1;
    }

    if (mr_start(mr, argv[1], argv[2]) == -1) {
        perror("mr_start");
        mr_destroy(mr);
        mr_attr_destroy(&attr);
        return 1;
    }

    mr_destroy(mr);
    mr_attr_destroy(&attr);
    return 0;
}
