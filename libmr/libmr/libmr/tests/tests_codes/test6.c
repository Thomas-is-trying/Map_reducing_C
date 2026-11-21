#include <stdio.h>
#include <stdlib.h>
#include "test_utils.h"
#include "test_mappers.h"

void test6_opaque_nulls(void) {
    printf("\n=== Test 6: value/result con byte nulli interni, multi-emit per token ===\n");
    system("mkdir -p build/it_input_nulls");
    FILE *f = fopen("build/it_input_nulls/lines.txt", "w");
    fprintf(f, "a\nb\nc\n");
    fclose(f);

    run_instance(nb_mapper, nb_reducer,
                 "build/it_input_nulls", "build/it_output_nulls.mro",
                 "build/logs/test6.log.txt", "build/stats/test6.stats.txt");

    long matches = count_matching_records("build/it_output_nulls.mro", "marker",
                                           NB_PAYLOAD, sizeof(NB_PAYLOAD));
    CHECK(matches == 3, "3 record con byte nulli interni, tutti integri e non troncati");

    long total_records = count_records("build/it_output_nulls.mro");
    CHECK(total_records == 3, "il reducer ha davvero emesso piu' risultati per lo stesso token");
}
