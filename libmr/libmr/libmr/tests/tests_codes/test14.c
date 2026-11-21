#include <stdio.h>
#include "test_utils.h"
#include "test_mappers.h"

void test14_tiny_queue(void) {
    printf("\n=== Test 14: code interne di capacita' minima (queue_size=1) ===\n");
    run_instance_full(it_mapper, it_reducer,
                       1, 1, 1, NULL, NULL,
                       "build/it_input_1", "build/it_output_tinyqueue.mro",
                       "build/logs/test14.log.txt", "build/stats/test14.stats.txt");

    long count;
    int found;

    found = find_token_count("build/it_output_tinyqueue.mro", "the", &count);
    CHECK(found == 1 && count == 5, "con coda di capacita' 1 e un solo thread, il risultato resta corretto");

    found = find_token_count("build/it_output_tinyqueue.mro", "fox", &count);
    CHECK(found == 1 && count == 3, "nessun deadlock ne' perdita di dati con code minime");
}
