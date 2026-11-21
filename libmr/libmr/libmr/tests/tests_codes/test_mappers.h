#ifndef TEST_MAPPERS_H
#define TEST_MAPPERS_H

#include "../../include/mr.h"

/* conteggio occorrenze: usato dalla maggior parte dei test "generici" */
int it_mapper(const mr_file_line_t *line, mr_emit_pair_t emit,
              void *emit_arg, void *user_arg);
int it_reducer(const char *token, const mr_value_t *values,
               size_t values_count, mr_emit_result_t emit,
               void *emit_arg, void *user_arg);

/* value/result a 5 byte con due '\0' interni (test 6) */
extern const unsigned char NB_PAYLOAD[5];
int nb_mapper(const mr_file_line_t *line, mr_emit_pair_t emit,
              void *emit_arg, void *user_arg);
int nb_reducer(const char *token, const mr_value_t *values,
               size_t values_count, mr_emit_result_t emit,
               void *emit_arg, void *user_arg);

/* value/result di dimensione 0 con puntatore NULL (test 9) */
int zero_mapper(const mr_file_line_t *line, mr_emit_pair_t emit,
                 void *emit_arg, void *user_arg);
int zero_reducer(const char *token, const mr_value_t *values,
                  size_t values_count, mr_emit_result_t emit,
                  void *emit_arg, void *user_arg);

/* value di dimensione variabile per lo stesso token (test 10) */
typedef struct { size_t total_size; unsigned char xor_byte; } value_check_t;

int var_mapper(const mr_file_line_t *line, mr_emit_pair_t emit,
               void *emit_arg, void *user_arg);
int var_reducer(const char *token, const mr_value_t *values,
                 size_t values_count, mr_emit_result_t emit,
                 void *emit_arg, void *user_arg);

/* reducer che non emette nulla se values_count e' pari (test 11) */
int skip_reducer(const char *token, const mr_value_t *values,
                  size_t values_count, mr_emit_result_t emit,
                  void *emit_arg, void *user_arg);

/* hash deliberatamente degenere: sempre bucket 0 (test 12) */
size_t const_hash(const char *token, size_t token_len, void *user_arg);

/* mapper che verifica line_number (test 13) */
int ln_mapper(const mr_file_line_t *line, mr_emit_pair_t emit,
              void *emit_arg, void *user_arg);

#endif
