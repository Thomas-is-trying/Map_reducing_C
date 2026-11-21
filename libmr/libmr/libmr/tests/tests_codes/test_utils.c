#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "test_utils.h"

int g_failures = 0;

/* ================= utility di parsing del formato .mro ================= */

int find_token_count(const char *mro_path, const char *target_token, long *out_count) {
    FILE *f = fopen(mro_path, "rb");
    if (f == NULL) { return -1; }

    size_t token_len, result_len;
    int found = 0;
    while (fread(&token_len, sizeof(token_len), 1, f) == 1) {
        char *token = malloc(token_len + 1);
        if (token == NULL) { fclose(f); return -1; }
        if (fread(token, token_len, 1, f) != 1) { free(token); break; }
        token[token_len] = '\0';

        if (fread(&result_len, sizeof(result_len), 1, f) != 1) { free(token); break; }
        if (result_len != sizeof(long)) { free(token); fclose(f); return -1; }

        long count;
        if (fread(&count, sizeof(count), 1, f) != 1) { free(token); break; }

        if (strcmp(token, target_token) == 0) {
            *out_count = count;
            found = 1;
        }
        free(token);
    }
    fclose(f);
    return found;
}

long count_records(const char *mro_path) {
    FILE *f = fopen(mro_path, "rb");
    if (f == NULL) { return -1; }

    size_t token_len, result_len;
    long count = 0;
    while (fread(&token_len, sizeof(token_len), 1, f) == 1) {
        if (fseek(f, (long)token_len, SEEK_CUR) != 0) { fclose(f); return -1; }
        if (fread(&result_len, sizeof(result_len), 1, f) != 1) { break; }
        if (fseek(f, (long)result_len, SEEK_CUR) != 0) { fclose(f); return -1; }
        count++;
    }
    fclose(f);
    return count;
}

long count_matching_records(const char *mro_path, const char *target_token,
                             const void *expected_bytes, size_t expected_len) {
    FILE *f = fopen(mro_path, "rb");
    if (f == NULL) { return -1; }

    size_t token_len, result_len;
    long matches = 0;
    while (fread(&token_len, sizeof(token_len), 1, f) == 1) {
        char *token = malloc(token_len + 1);
        if (token == NULL) { fclose(f); return -1; }
        if (fread(token, token_len, 1, f) != 1) { free(token); break; }
        token[token_len] = '\0';

        if (fread(&result_len, sizeof(result_len), 1, f) != 1) { free(token); break; }

        void *result = malloc(result_len > 0 ? result_len : 1);
        if (result == NULL) { free(token); fclose(f); return -1; }
        if (result_len > 0 && fread(result, result_len, 1, f) != 1) {
            free(token); free(result); break;
        }

        if (strcmp(token, target_token) == 0 &&
            result_len == expected_len &&
            memcmp(result, expected_bytes, expected_len) == 0) {
            matches++;
        }
        free(token);
        free(result);
    }
    fclose(f);
    return matches;
}

int get_nth_matching_record(const char *mro_path, const char *target_token, int n,
                             void **out_data, size_t *out_len) {
    FILE *f = fopen(mro_path, "rb");
    if (f == NULL) { return -1; }

    size_t token_len, result_len;
    int idx = -1;
    while (fread(&token_len, sizeof(token_len), 1, f) == 1) {
        char *token = malloc(token_len + 1);
        if (token == NULL) { fclose(f); return -1; }
        if (fread(token, token_len, 1, f) != 1) { free(token); break; }
        token[token_len] = '\0';

        if (fread(&result_len, sizeof(result_len), 1, f) != 1) { free(token); break; }

        void *data = NULL;
        if (result_len > 0) {
            data = malloc(result_len);
            if (data == NULL) { free(token); fclose(f); return -1; }
            if (fread(data, result_len, 1, f) != 1) { free(token); free(data); break; }
        }

        if (strcmp(token, target_token) == 0) {
            idx++;
            if (idx == n) {
                *out_data = data;
                *out_len = result_len;
                free(token);
                fclose(f);
                return 1;
            }
        }
        free(token);
        if (data) { free(data); }
    }
    fclose(f);
    return 0;
}

int parse_stats_field(const char *stats_path, const char *field_name, size_t *out_value) {
    FILE *f = fopen(stats_path, "r");
    if (f == NULL) { return -1; }

    char line[256];
    char prefix[128];
    snprintf(prefix, sizeof(prefix), "%s:", field_name);
    size_t prefix_len = strlen(prefix);

    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, prefix, prefix_len) == 0) {
            if (sscanf(line + prefix_len, " %zu", out_value) == 1) {
                found = 1;
            }
        }
    }
    fclose(f);
    return found;
}

int files_equal(const char *path_a, const char *path_b) {
    FILE *fa = fopen(path_a, "rb");
    FILE *fb = fopen(path_b, "rb");
    if (fa == NULL || fb == NULL) {
        if (fa) fclose(fa);
        if (fb) fclose(fb);
        return -1;
    }

    int result = 1;
    int ca, cb;
    do {
        ca = fgetc(fa);
        cb = fgetc(fb);
        if (ca != cb) { result = 0; break; }
    } while (ca != EOF && cb != EOF);

    fclose(fa);
    fclose(fb);
    return result;
}

int check_lexicographic_order(const char *mro_path) {
    FILE *f = fopen(mro_path, "rb");
    if (f == NULL) { return -1; }

    size_t token_len, result_len;
    char prev[1024];
    prev[0] = '\0';
    int first = 1;
    int ok = 1;

    while (fread(&token_len, sizeof(token_len), 1, f) == 1) {
        char *token = malloc(token_len + 1);
        if (token == NULL) { fclose(f); return -1; }
        if (fread(token, token_len, 1, f) != 1) { free(token); break; }
        token[token_len] = '\0';

        if (fread(&result_len, sizeof(result_len), 1, f) != 1) { free(token); break; }
        if (fseek(f, (long)result_len, SEEK_CUR) != 0) { free(token); fclose(f); return -1; }

        if (!first && strcmp(prev, token) > 0) {
            ok = 0;
            free(token);
            break;
        }
        strncpy(prev, token, sizeof(prev) - 1);
        prev[sizeof(prev) - 1] = '\0';
        first = 0;
        free(token);
    }
    fclose(f);
    return ok;
}

int file_contains_string(const char *path, const char *needle) {
    FILE *f = fopen(path, "rb");
    if (f == NULL) { return -1; }
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return -1; }
    long size = ftell(f);
    if (size < 0) { fclose(f); return -1; }
    rewind(f);

    char *buf = malloc((size_t)size + 1);
    if (buf == NULL) { fclose(f); return -1; }
    size_t rd = fread(buf, 1, (size_t)size, f);
    buf[rd] = '\0';
    fclose(f);

    int found = (strstr(buf, needle) != NULL);
    free(buf);
    return found;
}

/* Timestamp in millisecondi, per misurare informativamente la durata
 * dei test di volume (test 16/17). Non usato per far fallire i test:
 * la performance non è un criterio pass/fail qui, solo un dato
 * diagnostico stampato a video. */
double time_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
}

/* ================= setup di input condiviso ================= */

void write_base_input(const char *dir_path) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "mkdir -p %s", dir_path);
    system(cmd);

    char path1[256], path2[256];
    snprintf(path1, sizeof(path1), "%s/sample1.txt", dir_path);
    snprintf(path2, sizeof(path2), "%s/sample2.txt", dir_path);

    FILE *f1 = fopen(path1, "w");
    fprintf(f1, "the quick brown fox jumps over the lazy dog the fox runs\n");
    fclose(f1);

    FILE *f2 = fopen(path2, "w");
    fprintf(f2, "the dog barks at the fox\n");
    fclose(f2);
}

/* ================= esecuzione parametrizzata di un'elaborazione ================= */

void run_instance_full(mr_mapper_t mapper, mr_reducer_t reducer,
                        size_t mapper_threads, size_t reducer_threads, size_t queue_size,
                        mr_hash_t hash, void *hash_arg,
                        const char *input_path, const char *output_path,
                        const char *log_path, const char *stats_path) {
    mr_t mr;
    mr_attr_t attr;

    CHECK(mr_attr_init(&attr) == 0, "mr_attr_init riesce");
    CHECK(mr_attr_set_mapper_threads(&attr, mapper_threads) == 0, "set_mapper_threads riesce");
    CHECK(mr_attr_set_reducer_threads(&attr, reducer_threads) == 0, "set_reducer_threads riesce");
    CHECK(mr_attr_set_queue_size(&attr, queue_size) == 0, "set_queue_size riesce");
    CHECK(mr_attr_set_log_file(&attr, log_path) == 0, "set_log_file riesce");
    CHECK(mr_attr_set_stats_file(&attr, stats_path) == 0, "set_stats_file riesce");
    CHECK(mr_attr_set_hash_function(&attr, hash, hash_arg) == 0, "set_hash_function riesce");

    CHECK(mr_create(&mr, &attr, mapper, reducer, NULL) == 0, "mr_create riesce");
    CHECK(mr_start(mr, input_path, output_path) == 0, "mr_start riesce");

    mr_destroy(mr);
    mr_attr_destroy(&attr);
}

/* wrapper per i casi comuni: 3 thread mapper/reducer, coda 16, hash di default */
void run_instance(mr_mapper_t mapper, mr_reducer_t reducer,
                   const char *input_path, const char *output_path,
                   const char *log_path, const char *stats_path) {
    run_instance_full(mapper, reducer, 3, 3, 16, NULL, NULL,
                       input_path, output_path, log_path, stats_path);
}
