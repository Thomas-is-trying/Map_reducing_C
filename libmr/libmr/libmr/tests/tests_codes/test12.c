#include <stdio.h>
#include "test_utils.h"
#include "test_mappers.h"

void test12_custom_hash(void) {
    printf("\n=== Test 12: funzione di hash personalizzata (addendum) ===\n");
    run_instance_full(it_mapper, it_reducer,
                       3, 3, 16, const_hash, NULL,
                       "build/it_input_1", "build/it_output_hash.mro",
                       "build/logs/test12.log.txt", "build/stats/test12.stats.txt");

    long count;
    int found;

    found = find_token_count("build/it_output_hash.mro", "the", &count);
    CHECK(found == 1 && count == 5, "con hash personalizzato (degenere) il conteggio di 'the' resta 5");

    found = find_token_count("build/it_output_hash.mro", "fox", &count);
    CHECK(found == 1 && count == 3, "con hash personalizzato (degenere) il conteggio di 'fox' resta 3");
}
