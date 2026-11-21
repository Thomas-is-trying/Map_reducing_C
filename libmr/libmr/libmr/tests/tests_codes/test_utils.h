#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <stddef.h>
#include "../../include/mr.h"

/* ================= stato condiviso fra tutti i file di test ================= */

extern int g_failures;

#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s (linea %d)\n", msg, __LINE__); \
            g_failures++; \
        } else { \
            printf("PASS: %s\n", msg); \
        } \
    } while (0)

/* ================= utility di parsing del formato .mro / stats / log ================= */

int find_token_count(const char *mro_path, const char *target_token, long *out_count);
long count_records(const char *mro_path);
long count_matching_records(const char *mro_path, const char *target_token,
                             const void *expected_bytes, size_t expected_len);
int get_nth_matching_record(const char *mro_path, const char *target_token, int n,
                             void **out_data, size_t *out_len);
int parse_stats_field(const char *stats_path, const char *field_name, size_t *out_value);
int files_equal(const char *path_a, const char *path_b);
int check_lexicographic_order(const char *mro_path);
int file_contains_string(const char *path, const char *needle);
double time_now_ms(void);

/* ================= setup di input ed esecuzione condivisi ================= */

void write_base_input(const char *dir_path);

void run_instance_full(mr_mapper_t mapper, mr_reducer_t reducer,
                        size_t mapper_threads, size_t reducer_threads, size_t queue_size,
                        mr_hash_t hash, void *hash_arg,
                        const char *input_path, const char *output_path,
                        const char *log_path, const char *stats_path);

void run_instance(mr_mapper_t mapper, mr_reducer_t reducer,
                   const char *input_path, const char *output_path,
                   const char *log_path, const char *stats_path);

#endif
