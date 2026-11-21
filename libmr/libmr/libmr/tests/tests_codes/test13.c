#include <stdio.h>
#include <stdlib.h>
#include "test_utils.h"
#include "test_mappers.h"

void test13_line_numbering(void) {
    printf("\n=== Test 13: numerazione delle righe, nessuna perdita/duplicazione ===\n");
    system("mkdir -p build/it_input_lines");
    FILE *f = fopen("build/it_input_lines/five.txt", "w");
    fprintf(f, "one\ntwo\nthree\nfour\nfive\n");
    fclose(f);

    run_instance(ln_mapper, it_reducer,
                 "build/it_input_lines", "build/it_output_lines.mro",
                 "build/logs/test13.log.txt", "build/stats/test13.stats.txt");

    long count;
    int found;
    const char *expected_tokens[] = { "ln1", "ln2", "ln3", "ln4", "ln5" };
    int all_ok = 1;
    for (int i = 0; i < 5; i++) {
        found = find_token_count("build/it_output_lines.mro", expected_tokens[i], &count);
        if (!(found == 1 && count == 1)) { all_ok = 0; }
    }
    CHECK(all_ok, "righe numerate 1..5 tutte presenti esattamente una volta, nessuna persa o duplicata");

    long total = count_records("build/it_output_lines.mro");
    CHECK(total == 5, "esattamente 5 record in output, nessun token estraneo");
}
