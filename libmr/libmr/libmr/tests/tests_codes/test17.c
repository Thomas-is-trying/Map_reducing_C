#include <stdio.h>
#include <stdlib.h>
#include "test_utils.h"
#include "test_mappers.h"

void test17_large_repeated(void) {
    printf("\n=== Test 17: file di grandi dimensioni con vocabolario ripetuto (throughput) ===\n");
    system("mkdir -p build/it_input_large_repeated");
    const int VOCAB = 50;
    const int LINES = 20000;
    const int WORDS_PER_LINE = 5;
    long expected_counts[VOCAB];
    for (int i = 0; i < VOCAB; i++) { expected_counts[i] = 0; }

    FILE *f = fopen("build/it_input_large_repeated/data.txt", "w");
    for (int i = 0; i < LINES; i++) {
        for (int j = 0; j < WORDS_PER_LINE; j++) {
            int idx = (i * WORDS_PER_LINE + j) % VOCAB;
            fprintf(f, "w%02d", idx);
            expected_counts[idx]++;
            if (j < WORDS_PER_LINE - 1) { fprintf(f, " "); }
        }
        fprintf(f, "\n");
    }
    fclose(f);

    double t0 = time_now_ms();
    run_instance(it_mapper, it_reducer,
                 "build/it_input_large_repeated", "build/it_output_large_repeated.mro",
                 "build/logs/test17.log.txt", "build/stats/test17.stats.txt");
    double t1 = time_now_ms();
    printf("INFO: elaborazione di %d righe (%d coppie) completata in %.1f ms\n",
           LINES, LINES * WORDS_PER_LINE, t1 - t0);

    long records = count_records("build/it_output_large_repeated.mro");
    CHECK(records == VOCAB, "tutti i token del vocabolario presenti, nessuno perso o spurio");

    int all_correct = 1;
    for (int i = 0; i < VOCAB; i++) {
        char token[8];
        snprintf(token, sizeof(token), "w%02d", i);
        long count;
        int found = find_token_count("build/it_output_large_repeated.mro", token, &count);
        if (!(found == 1 && count == expected_counts[i])) {
            all_correct = 0;
            fprintf(stderr, "  mismatch per '%s': atteso %ld, trovato %s\n",
                    token, expected_counts[i], found ? "valore diverso" : "non trovato");
        }
    }
    CHECK(all_correct, "tutti i 50 conteggi (calcolati a runtime durante la generazione dell'input) combaciano esattamente");
}
