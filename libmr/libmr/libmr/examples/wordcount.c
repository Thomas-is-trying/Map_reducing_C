#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "../include/mr.h"

//Mapper: estrae token alfanumerici dalla riga ed emette <token, 1> per ognuno.
static int wc_mapper(const mr_file_line_t *line, mr_emit_pair_t emit, void *emit_arg, void *user_arg) {
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

// Reducer: somma tutti i valori interi associati allo stesso token ed emette il totale.

static int wc_reducer(const char *token, const mr_value_t *values,size_t values_count, mr_emit_result_t emit,void *emit_arg, void *user_arg) {
    (void)user_arg;
    long total = 0;
    for (size_t i = 0; i < values_count; i++) {
        total += *(const int *)values[i].data;
    }
    return emit(token, &total, sizeof(total), emit_arg);
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
    if (mr_attr_set_log_file(&attr, "mr.log") == -1) { perror("log_file"); return 1; }
    if (mr_attr_set_stats_file(&attr, "mr.stats") == -1) { perror("stats_file"); return 1; }

    if (mr_create(&mr, &attr, wc_mapper, wc_reducer, NULL) == -1) {
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