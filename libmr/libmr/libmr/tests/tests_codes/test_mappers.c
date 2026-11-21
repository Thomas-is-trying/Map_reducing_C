#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "test_mappers.h"

/* ================= conteggio occorrenze (condiviso) ================= */

int it_mapper(const mr_file_line_t *line, mr_emit_pair_t emit,
              void *emit_arg, void *user_arg) {
    (void)user_arg;
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
            int one = 1;
            if (emit(token, &one, sizeof(one), emit_arg) == -1) {
                return -1;
            }
        } else {
            i++;
        }
    }
    return 0;
}

int it_reducer(const char *token, const mr_value_t *values,
               size_t values_count, mr_emit_result_t emit,
               void *emit_arg, void *user_arg) {
    (void)user_arg;
    long total = 0;
    for (size_t i = 0; i < values_count; i++) {
        total += *(const int *)values[i].data;
    }
    return emit(token, &total, sizeof(total), emit_arg);
}

/* ================= byte nulli interni + multi-emit (test 6) ================= */

const unsigned char NB_PAYLOAD[5] = { 'A', '\0', 'B', '\0', 'C' };

int nb_mapper(const mr_file_line_t *line, mr_emit_pair_t emit,
              void *emit_arg, void *user_arg) {
    (void)user_arg;
    if (line->line_len == 0) { return 0; }
    return emit("marker", NB_PAYLOAD, sizeof(NB_PAYLOAD), emit_arg);
}

int nb_reducer(const char *token, const mr_value_t *values,
               size_t values_count, mr_emit_result_t emit,
               void *emit_arg, void *user_arg) {
    (void)user_arg;
    for (size_t i = 0; i < values_count; i++) {
        if (emit(token, values[i].data, values[i].size, emit_arg) == -1) {
            return -1;
        }
    }
    return 0;
}

/* ================= value/result dimensione 0 (test 9) ================= */

int zero_mapper(const mr_file_line_t *line, mr_emit_pair_t emit,
                 void *emit_arg, void *user_arg) {
    (void)user_arg;
    if (line->line_len == 0) { return 0; }
    return emit("marker0", NULL, 0, emit_arg);
}

int zero_reducer(const char *token, const mr_value_t *values,
                  size_t values_count, mr_emit_result_t emit,
                  void *emit_arg, void *user_arg) {
    (void)user_arg; (void)values;
    int cnt = (int)values_count;
    if (emit(token, &cnt, sizeof(cnt), emit_arg) == -1) { return -1; }
    if (emit(token, NULL, 0, emit_arg) == -1) { return -1; }
    return 0;
}

/* ================= value di dimensione variabile (test 10) ================= */

int var_mapper(const mr_file_line_t *line, mr_emit_pair_t emit,
               void *emit_arg, void *user_arg) {
    (void)user_arg;
    if (line->line_len == 0) { return 0; }
    return emit("varsize", line->line, line->line_len, emit_arg);
}

int var_reducer(const char *token, const mr_value_t *values,
                 size_t values_count, mr_emit_result_t emit,
                 void *emit_arg, void *user_arg) {
    (void)user_arg;
    value_check_t vc;
    memset(&vc, 0, sizeof(vc)); /* azzera anche il padding di allineamento
                                  * fra total_size e xor_byte, altrimenti
                                  * quei byte restano non inizializzati */
    vc.total_size = 0;
    vc.xor_byte = 0;
    for (size_t i = 0; i < values_count; i++) {
        vc.total_size += values[i].size;
        const unsigned char *bytes = (const unsigned char *)values[i].data;
        for (size_t j = 0; j < values[i].size; j++) {
            vc.xor_byte ^= bytes[j];
        }
    }
    return emit(token, &vc, sizeof(vc), emit_arg);
}

/* ================= reducer che puo' emettere zero risultati (test 11) ================= */

int skip_reducer(const char *token, const mr_value_t *values,
                  size_t values_count, mr_emit_result_t emit,
                  void *emit_arg, void *user_arg) {
    (void)user_arg;
    if (values_count % 2 == 0) {
        return 0; /* nessun emit: il reducer puo' emettere zero risultati */
    }
    long total = 0;
    for (size_t i = 0; i < values_count; i++) {
        total += *(const int *)values[i].data;
    }
    return emit(token, &total, sizeof(total), emit_arg);
}

/* ================= hash personalizzata degenere (test 12) ================= */

size_t const_hash(const char *token, size_t token_len, void *user_arg) {
    (void)token; (void)token_len; (void)user_arg;
    return 0;
}

/* ================= mapper che verifica line_number (test 13) ================= */

int ln_mapper(const mr_file_line_t *line, mr_emit_pair_t emit,
              void *emit_arg, void *user_arg) {
    (void)user_arg;
    char buf[32];
    snprintf(buf, sizeof(buf), "ln%lu", line->line_number);
    int one = 1;
    return emit(buf, &one, sizeof(one), emit_arg);
}
