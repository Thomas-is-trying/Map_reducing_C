#include <stdio.h>
#include "test_utils.h"
#include "test_mappers.h"

void test11_zero_results(void) {
    printf("\n=== Test 11: reducer che emette zero risultati per alcuni token ===\n");
    run_instance(it_mapper, skip_reducer,
                 "build/it_input_1", "build/it_output_skip.mro",
                 "build/logs/test11.log.txt", "build/stats/test11.stats.txt");

    long count;
    int found;

    found = find_token_count("build/it_output_skip.mro", "dog", &count);
    CHECK(found == 0, "token con conteggio pari ('dog') non compare: il reducer ha emesso zero risultati");

    found = find_token_count("build/it_output_skip.mro", "the", &count);
    CHECK(found == 1 && count == 5, "token con conteggio dispari ('the') e' comunque emesso correttamente");

    found = find_token_count("build/it_output_skip.mro", "fox", &count);
    CHECK(found == 1 && count == 3, "token con conteggio dispari ('fox') e' comunque emesso correttamente");
}
