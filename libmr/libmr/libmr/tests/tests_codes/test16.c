#include <stdio.h>
#include <stdlib.h>
#include "test_utils.h"
#include "test_mappers.h"

void test16_many_distinct_tokens(void) {
    printf("\n=== Test 16: molti token distinti (stress ricerca lineare nel raggruppamento) ===\n");
    system("mkdir -p build/it_input_many_distinct");
    const int N = 3000;
    FILE *f = fopen("build/it_input_many_distinct/data.txt", "w");
    for (int i = 0; i < N; i++) {
        fprintf(f, "tok%05d\n", i);
    }
    fclose(f);

    double t0 = time_now_ms();
    run_instance(it_mapper, it_reducer,
                 "build/it_input_many_distinct", "build/it_output_many_distinct.mro",
                 "build/logs/test16.log.txt", "build/stats/test16.stats.txt");
    double t1 = time_now_ms();
    printf("INFO: elaborazione di %d token distinti completata in %.1f ms\n", N, t1 - t0);

    long records = count_records("build/it_output_many_distinct.mro");
    CHECK(records == N, "tutti gli N token distinti sono presenti nell'output, nessuna perdita/duplicazione");

    long count;
    int found;
    found = find_token_count("build/it_output_many_distinct.mro", "tok00000", &count);
    CHECK(found == 1 && count == 1, "primo token ('tok00000') presente con conteggio corretto");
    found = find_token_count("build/it_output_many_distinct.mro", "tok01500", &count);
    CHECK(found == 1 && count == 1, "token centrale ('tok01500') presente con conteggio corretto");
    char last_tok[16];
    snprintf(last_tok, sizeof(last_tok), "tok%05d", N - 1);
    found = find_token_count("build/it_output_many_distinct.mro", last_tok, &count);
    CHECK(found == 1 && count == 1, "ultimo token presente con conteggio corretto");
}
