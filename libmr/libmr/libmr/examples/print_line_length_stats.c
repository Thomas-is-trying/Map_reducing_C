#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Programma ausiliario per stampare in forma leggibile il file .mro
 * prodotto da line_length_stats.c.
 *
 * Formato record letto (coerente con sez. 8 della specifica e con
 * mr_receive_from_reducer): [size_t token_len][token][size_t result_len][result]
 * dove result e' una lls_result_t{ int tag; size_t value; } (vedi
 * line_length_stats.c per la definizione esatta).
 *
 * Uso: print_line_length_stats <file.mro>
 */

typedef enum { LLS_TAG_MIN = 0, LLS_TAG_MAX = 1 } lls_tag_t;
typedef struct { lls_tag_t tag; size_t value; } lls_result_t;

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "uso: %s <file.mro>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (f == NULL) { perror("fopen"); return 1; }

    size_t token_len, result_len;
    while (fread(&token_len, sizeof(token_len), 1, f) == 1) {
        char *token = malloc(token_len + 1);
        if (token == NULL) { fclose(f); return 1; }
        if (fread(token, token_len, 1, f) != 1) { free(token); break; }
        token[token_len] = '\0';

        if (fread(&result_len, sizeof(result_len), 1, f) != 1) { free(token); break; }

        if (result_len != sizeof(lls_result_t)) {
            fprintf(stderr, "record inatteso per token '%s': result_len=%zu\n", token, result_len);
            free(token);
            break;
        }

        lls_result_t r;
        if (fread(&r, sizeof(r), 1, f) != 1) { free(token); break; }

        printf("%-20s %-4s %zu\n", token, r.tag == LLS_TAG_MIN ? "min" : "max", r.value);
        free(token);
    }

    fclose(f);
    return 0;
}
